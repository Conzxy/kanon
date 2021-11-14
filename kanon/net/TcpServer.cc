#include "kanon/net/TcpServer.h"
#include "kanon/net/InetAddr.h"
#include "kanon/net/TcpConnection.h"
#include "kanon/net/EventLoop.h"
#include "kanon/net/EventLoopPool.h"
#include "kanon/net/Acceptor.h"
#include "kanon/util/unique_ptr.h"

using namespace kanon;

TcpServer::TcpServer(EventLoop* loop,
           InetAddr const& listen_addr,
           StringArg name,
           bool reuseport)
  : loop_{ loop }
  , ip_port_{ listen_addr.toIpPort() }
  , name_{ name }
  , acceptor_{ kanon::make_unique<Acceptor>(loop_, listen_addr, reuseport) }
  , next_conn_id{ 1 }
  , pool_{ kanon::make_unique<EventLoopPool>(
       loop, static_cast<char const*>(name)) }
{
  setConnectionCallback(&defaultConnectionCallaback);

  acceptor_->setNewConnectionCallback([this](int cli_sock, InetAddr const& cli_addr) {
    // ensure in main thread
    loop_->assertInThread();
    
    // use eventloop pool and robin-round to choose a IO loop
    //
    char buf[64];
    ::snprintf(buf, sizeof buf, "-%s#%u", ip_port_.c_str(), next_conn_id);
    ++next_conn_id;
    auto conn_name = name_ + buf;
    
    auto io_loop = pool_->getNextLoop();

    auto local_addr = sock::getLocalAddr(cli_sock);
    auto conn = std::make_shared<TcpConnection>(io_loop,
                          conn_name,
                          cli_sock,
                          local_addr,
                          cli_addr);  

    connections_[conn_name] = conn;

    conn->setMessageCallback(message_callback_);
    conn->setConnectionCallback(connection_callback_);
    conn->setWriteCompleteCallback(write_complete_callback_);
    conn->setCloseCallback([this](TcpConnectionPtr const& conn) {
        this->removeConnection(conn);
    });
    
    // io loop or main loop
    io_loop->runInLoop([conn]() {
      conn->connectionEstablished();
    });

  });
  
  //acceptor_.listen();
}

TcpServer::~TcpServer() KANON_NOEXCEPT
{
  for (auto& conn_pair : connections_) {
    auto conn = conn_pair.second;
    conn_pair.second.reset();

    auto io_loop = conn->loop();
    io_loop->runInLoop([&conn]() {
        conn->connectionDestroyed();
    });
  }
}

void
TcpServer::setLoopNum(int num) KANON_NOEXCEPT {
  pool_->setLoopNum(num);
}

void
TcpServer::start() KANON_NOEXCEPT {
  // !Use count_ instead of atomic variable
  // if (! atomic_bool) {
  //   atomic_bool = true;
  //   // do something
  // }
  // If there is a thread trigger an interrput
  // atomic_bool is false still, so other thread also enter if block
  if (count_.getAndSet(1) == 0) {
    // start IO loop
    pool_->start();

    acceptor_->listen();
  }
}

void
TcpServer::removeConnection(TcpConnectionPtr const& conn) {
  // remove connection in server thread
  loop_->runInLoop([conn, this]() {
    loop_->assertInThread();

    auto n = connections_.erase(conn->name());
    assert(n == 1);
    KANON_UNUSED(n);
    
    auto io_loop = conn->loop();
    io_loop->runInLoop([conn]() {
        conn->connectionDestroyed();
    });
  });
}