#include "kanon/net/tcp_server.h"

#include "kanon/util/macro.h"
#include "kanon/util/ptr.h"

#include "kanon/net/inet_addr.h"
#include "kanon/net/connection/tcp_connection.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/event_loop_pool.h"
#include "kanon/net/acceptor.h"

#include "kanon/mem/object_pool_allocator.h"

#include <signal.h>
#include <atomic>

using namespace kanon;

static EventLoop *g_loop = nullptr;

KANON_INLINE void SignalHandler(int signo, int expected,
                                char const *msg) KANON_NOEXCEPT
{
  KANON_UNUSED(signo);
  assert(signo == expected);
  LOG_INFO_KANON << msg;

  if (g_loop) {
    LOG_INFO_KANON << "The event loop is quiting...";
    g_loop->Quit();
  }

  LOG_INFO_KANON << "The server exit successfully";
}

#ifdef KANON_ON_UNIX
KANON_INLINE void SigKillHandler(int signo) KANON_NOEXCEPT
{
  SignalHandler(signo, SIGKILL, "Catching SIGKILL, force kill the server");
  exit(1);
}
#endif

KANON_INLINE void SigIntHandler(int signo) KANON_NOEXCEPT
{
  SignalHandler(signo, SIGINT,
                "Catching SIGINT(i.e. User type Ctrl+c), server is exiting");
  exit(0);
}

KANON_INLINE void SigTermHandler(int signo) KANON_NOEXCEPT
{
  SignalHandler(signo, SIGTERM,
                "Catching SIGTERM(e.g. kill(1)), server is terminated");
  exit(1);
}

TcpServer::TcpServer(EventLoop *loop, InetAddr const &listen_addr,
                     StringArg name, bool reuseport)
  : loop_{loop}
  , ip_port_{listen_addr.ToIpPort()}
  , name_{name}
  , acceptor_{kanon::make_unique<Acceptor>(loop_, listen_addr, reuseport)}
  , connection_callback_(&DefaultConnectionCallback)
  , next_conn_id{1}
  , pool_{kanon::make_unique<EventLoopPool>(loop,
                                            static_cast<char const *>(name))}
  , start_once_{false}
  , enable_pool_{false}
  , conn_pool_{10}
{
  g_loop = loop_;
  ::signal(SIGINT, &SigIntHandler);
  ::signal(SIGTERM, &SigTermHandler);
#ifdef KANON_ON_UNIX
  ::signal(SIGKILL, &SigKillHandler);
#endif

  acceptor_->SetNewConnectionCallback(
      [this](int cli_sock, InetAddr const &cli_addr) {
        // Ensure in main thread
        loop_->AssertInThread();

        // use eventloop pool and robin-round to choose a IO loop
        char buf[64];
        ::snprintf(buf, sizeof buf, "-%s#%u", ip_port_.c_str(), next_conn_id);
        ++next_conn_id;
        auto conn_name = name_ + buf;

        auto io_loop = pool_->GetNextLoop();
        auto local_addr = sock::GetLocalAddr(cli_sock);

#if 0
    TcpConnectionPtr conn;
    if (conn_pool_.IsEnabled()) {
      TcpConnection *raw_conn;
      if (conn_pool_.Pop(raw_conn)) {
        raw_conn->InplaceConstruct(io_loop, conn_name, cli_sock, local_addr, cli_addr);
        conn.reset(raw_conn);
      } else {
        conn = TcpConnection::NewTcpConnectionRaw(io_loop, conn_name, cli_sock, local_addr, cli_addr); 
      }
    } else {
      conn = TcpConnection::NewTcpConnection(io_loop, conn_name, cli_sock, local_addr, cli_addr);  
    }
#else
        LOG_TRACE_KANON << "Pool block num = " << conn_pool_.GetBlockNum();
        LOG_TRACE_KANON << "Pool usage = "
                        << conn_pool_.GetUsage(sizeof(TcpConnection));
        LOG_TRACE_KANON << "The number of alive connections: "
                        << connections_.size();

        auto conn =
            enable_pool_
                ? TcpConnection::NewTcpConnection(
                      io_loop, conn_name, cli_sock, local_addr, cli_addr,
                      ObjectPoolAllocator<void>(conn_pool_))
                : TcpConnection::NewTcpConnection(io_loop, conn_name, cli_sock,
                                                  local_addr, cli_addr);
#endif

        {
          MutexGuard guard(lock_conn_);
          connections_[conn_name] = conn;
        }

        conn->SetMessageCallback(message_callback_);
        conn->SetConnectionCallback(connection_callback_);
        conn->SetWriteCompleteCallback(write_complete_callback_);
        conn->SetCloseCallback([this](TcpConnectionPtr const &conn) {
          auto io_loop = conn->GetLoop();
          io_loop->AssertInThread();

          size_t n = 0;
          KANON_UNUSED(n);
          {
            MutexGuard guard(lock_conn_);
            n = connections_.erase(conn->GetName());
            LOG_TRACE_KANON << "The number of alive connections(closing): "
                            << connections_.size();
          }

          assert(n == 1);

          // !Must call QueueToLoop() here,
          // we can't destroy the channel_ in the handling events phase,
          // and delay the ConnectionDestroyed() in calling functor phase.
          io_loop->QueueToLoop([conn]() {
            conn->ConnectionDestroyed();
#if 0
          if (conn.use_count() != 1) return;
          if (conn_pool_.IsFull()) {
            LOG_TRACE_KANON << "Delete the connection since pool is full";
            delete conn.get();
          } else {
            LOG_TRACE_KANON << "Push the connection to pool";
            auto raw_conn = conn.get();
            raw_conn->~TcpConnection();
            conn_pool_.Push(raw_conn);
          }
#endif
          });
        });

        // io loop or main loop
        io_loop->RunInLoop([conn]() {
          conn->ConnectionEstablished();
        });
      });
}

TcpServer::~TcpServer() KANON_NOEXCEPT
{
  //
  for (auto &conn_pair : connections_) {
    auto conn = conn_pair.second;
    conn_pair.second.reset();

    auto io_loop = conn->GetLoop();
    io_loop->RunInLoop([conn]() {
      conn->ConnectionDestroyed();
    });
  }
}

void TcpServer::SetLoopNum(int num) KANON_NOEXCEPT { pool_->SetLoopNum(num); }

void TcpServer::StartRun() KANON_NOEXCEPT
{
  // ! Must be thread-safe
  // Scenario:
  // If the other thread also call this function,
  // may be call this more than once.
  // EventLoopPool::StartRun() can only be called once
  if (!start_once_) {
    // start IO loop
    loop_->RunInLoop([this]() {
      // Boot the loop pool in the binded loop
      // to avoid block the thread that call StartRun()
      // e.g.
      // EventLoop loop;
      // TcpServer server(&loop, addr);
      // Server.StartRun();
      // EventLoopThread loop_thr("Thread2");
      // TcpServer server2(loop_thr.StartRun());
      // server2.StartRun(); // initialize in the other thread
      //
      // Through base_loop_->AssertInLoop() force it.
      if (init_cb_ && pool_->GetLoopNum() == 0) {
        init_cb_(loop_);
      }
      pool_->StartRun(init_cb_);
      LOG_INFO_KANON << "Listening in " << ip_port_;
      acceptor_->Listen();
      start_once_ = true;
    });
  }
}

bool TcpServer::IsRunning() KANON_NOEXCEPT { return acceptor_->Listening(); }
