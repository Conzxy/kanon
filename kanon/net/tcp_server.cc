#include "kanon/net/tcp_server.h"

#include "kanon/util/macro.h"
#include "kanon/util/ptr.h"

#include "kanon/net/inet_addr.h"
#include "kanon/net/connection/tcp_connection.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/event_loop_pool.h"
#include "kanon/net/acceptor.h"

#include <signal.h>
#include <atomic>

using namespace kanon;

EventLoop* g_loop = nullptr;

inline void SignalHandler(int signo, int expected, char const* msg) noexcept {
  assert(signo == expected);
  LOG_INFO << msg;

  if (g_loop) {
    LOG_INFO << "The event loop is quiting...";
    g_loop->Quit();
  }

  LOG_INFO << "The server exit successfully";
}

inline void SigKillHandler(int signo) noexcept {
  SignalHandler(signo, SIGKILL, "Catching SIGKILL, force kill the server");
}

inline void SigIntHandler(int signo) noexcept {
  SignalHandler(signo, SIGINT, "Catching SIGINT(i.e. User type Ctrl+c), server is exiting");
}

inline void SigTermHandler(int signo) noexcept {
  SignalHandler(signo, SIGTERM, "Catching SIGTERM(e.g. kill(1)), server is terminated");
}

TcpServer::TcpServer(EventLoop* loop,
                     InetAddr const& listen_addr,
                     StringArg name,
                     bool reuseport)
  : loop_{ loop }
  , ip_port_{ listen_addr.ToIpPort() }
  , name_{ name }
  , acceptor_{ kanon::make_unique<Acceptor>(loop_, listen_addr, reuseport) }
  , connection_callback_(&DefaultConnectionCallback)
  , next_conn_id{ 1 }
  , pool_{ kanon::make_unique<EventLoopPool>(loop, static_cast<char const*>(name)) }
  , start_once_{ ATOMIC_FLAG_INIT }
{

  g_loop = loop_;
  ::signal(SIGINT, &SigIntHandler);
  ::signal(SIGTERM, &SigTermHandler);
  ::signal(SIGKILL, &SigKillHandler);

  acceptor_->SetNewConnectionCallback([this](int cli_sock, InetAddr const& cli_addr) {
    // Ensure in main thread
    loop_->AssertInThread();
    
    // use eventloop pool and robin-round to choose a IO loop
    char buf[64];
    ::snprintf(buf, sizeof buf, "-%s#%u", ip_port_.c_str(), next_conn_id);
    ++next_conn_id;
    auto conn_name = name_ + buf;
    
    auto io_loop = pool_->GetNextLoop();

    auto local_addr = sock::GetLocalAddr(cli_sock);
    auto conn = TcpConnection::NewTcpConnection(io_loop, conn_name, cli_sock, local_addr, cli_addr);  

    {
    MutexGuard guard(lock_conn_);
    connections_[conn_name] = conn;
    }

    conn->SetMessageCallback(message_callback_);
    conn->SetConnectionCallback(connection_callback_);
    conn->SetWriteCompleteCallback(write_complete_callback_);
    conn->SetCloseCallback([this](TcpConnectionPtr const& conn) {
      auto io_loop = conn->GetLoop();
      io_loop->AssertInThread();

      int n = 0; KANON_UNUSED(n);
      {
      MutexGuard guard(lock_conn_);
      n = connections_.erase(conn->GetName());
      }

      assert(n == 1);

      // !Must call QueueToLoop() here,
      // we can't destroy the channel_ in the handling events phase,
      // and delay the ConnectionDestroyed() in calling functor phase.
      io_loop->QueueToLoop([conn]() {
          conn->ConnectionDestroyed();
      });
    });
    
    // io loop or main loop
    io_loop->RunInLoop([conn]() {
      conn->ConnectionEstablished();
    });
  });
}

TcpServer::~TcpServer() noexcept
{
  // 
  for (auto& conn_pair : connections_) {
    auto conn = conn_pair.second;
    conn_pair.second.reset();

    auto io_loop = conn->GetLoop();
    io_loop->RunInLoop([conn]() {
        conn->ConnectionDestroyed();
    });
  }
}

void TcpServer::SetLoopNum(int num) noexcept {
  pool_->SetLoopNum(num);
}

void TcpServer::StartRun() noexcept {
  // ! Must be thread-safe
  // Scenario:
  // If the other thread also call this function,
  // may be call this more than once.
  // EventLoopPool::StartRun() can only be called once
  if (start_once_.test_and_set(std::memory_order_relaxed) == false) {
    // start IO loop
    pool_->StartRun();
    loop_->RunInLoop([this]() {
      LOG_INFO << "Listening in " << ip_port_;
      acceptor_->Listen();
    });
  }
}

bool TcpServer::IsRunning() noexcept {
  return acceptor_->Listening();
}
