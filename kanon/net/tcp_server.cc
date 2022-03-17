#include "kanon/net/tcp_server.h"

#include "kanon/util/ptr.h"

#include "kanon/net/inet_addr.h"
#include "kanon/net/tcp_connection.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/event_loop_pool.h"
#include "kanon/net/acceptor.h"

using namespace kanon;

TcpServer::TcpServer(EventLoop* loop,
           InetAddr const& listen_addr,
           StringArg name,
           bool reuseport)
  : loop_{ loop }
  , ip_port_{ listen_addr.ToIpPort() }
  , name_{ name }
  , acceptor_{ kanon::make_unique<Acceptor>(loop_, listen_addr, reuseport) }
  , next_conn_id{ 1 }
  , pool_{ kanon::make_unique<EventLoopPool>(
       loop, static_cast<char const*>(name)) }
  , start_once_{ ATOMIC_FLAG_INIT }
{
  SetConnectionCallback(&DefaultConnectionCallback);

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
    auto conn = std::make_shared<TcpConnection>(io_loop, conn_name, cli_sock, local_addr, cli_addr);  

    connections_[conn_name] = conn;

    conn->SetMessageCallback(message_callback_);
    conn->SetConnectionCallback(connection_callback_);
    conn->SetWriteCompleteCallback(write_complete_callback_);
    conn->SetCloseCallback([this](TcpConnectionPtr const& conn) {
      this->RemoveConnection(conn);
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
    io_loop->RunInLoop([&conn]() {
        conn->ConnectionDestroyed();
    });
  }
}

void
TcpServer::SetLoopNum(int num) noexcept {
  pool_->SetLoopNum(num);
}

void
TcpServer::StartRun() noexcept {
  // ! Must be thread-safe
  // Scenario:
  // If the other thread also call this function,
  // may be call this more than once.
  // EventLoopPool::StartRun() can only be called once
  if (start_once_.test_and_set(std::memory_order_relaxed) == false) {
    // start IO loop
    pool_->StartRun();
    loop_->RunInLoop(
      [this]() {
        LOG_TRACE << "Start listen in " << ip_port_;
        acceptor_->Listen();
      });
  }
}

void
TcpServer::RemoveConnection(TcpConnectionPtr const& conn) {
  // Remove connection in server thread
  loop_->RunInLoop([conn, this]() {
    loop_->AssertInThread();

    auto n = connections_.erase(conn->GetName());
    assert(n == 1);
    KANON_UNUSED(n);
    
    auto io_loop = conn->GetLoop();

    // !Must call QueueToLoop() here,
    // we can't destroy the channel_ in the handling events phase,
    // and delay the ConnectionDestroyed() in calling functor phase.
    io_loop->QueueToLoop([conn]() {
        conn->ConnectionDestroyed();
    });
  });
}