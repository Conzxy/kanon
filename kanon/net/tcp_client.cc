#include "kanon/net/tcp_client.h"

#include "kanon/net/event_loop.h"
#include "kanon/net/inet_addr.h"
#include "kanon/net/connector.h"
#include "kanon/net/tcp_connection.h"
#include "kanon/net/sock_api.h"

using namespace kanon;

TcpClient::TcpClient(
  EventLoop* loop,
  InetAddr const& serv_addr,
  std::string const& name)
  : loop_{ loop }
  // Use std::make_shared is ok here since no weak pointer
  , connector_{ std::make_shared<Connector>(loop, serv_addr) }
  , name_{ name }
  , connection_callback_{ &DefaultConnectionCallback }
  , connect_{ true }
  , retry_{ false }
  , conn_{ nullptr }
  , mutex_{ }
{
  LOG_TRACE_KANON << "TcpClient-[" << name_ << "]" << " constructed";

  connector_->SetNewConnectionCallback([this, &serv_addr](int sockfd) {
    if (conn_) { return ; }

    auto new_conn = TcpConnection::NewTcpConnection(loop_, name_, sockfd, sock::GetLocalAddr(sockfd), serv_addr);

    new_conn->SetMessageCallback(message_callback_);
    new_conn->SetWriteCompleteCallback(write_complete_callback_);
    new_conn->SetConnectionCallback(connection_callback_);
  
    // If we directly contruct and set new connection,
    // that is dangerous! we may be get a half-completed connection.
    {
      MutexGuard guard{ mutex_ };
      conn_ = new_conn;
    }

    // RemoveConnection
    conn_->SetCloseCallback([this](TcpConnectionPtr const& conn) {
      loop_->AssertInThread();
      // passive close connection
      assert(conn == conn_);
      assert(conn->GetLoop() == loop_);


      // Like TcpServer, we remove connection from TcpClient 
      {
        MutexGuard guard{ mutex_ };
        conn_.reset();
      } 
  
      // ! In event handling phase, don't remove channel(disable is allowed)
      // ! conn must be copied(\see TcoConnection::HandleClose())
      loop_->QueueToLoop([conn] () {
        conn->ConnectionDestroyed();
      });

      // If user does not call Disconnect() and
      // support restart when passive close occurred
      if (connect_ && retry_) {
        connector_->Restrat();
      }
    });
  
    // enable reading
    conn_->ConnectionEstablished();
  });
} 

TcpClient::~TcpClient() noexcept {
  LOG_TRACE_KANON << "TcpClient-[" << name_ << "]" << " destructed";

  bool is_unique = false;
  TcpConnectionPtr conn;
  {
    MutexGuard guard{ mutex_ };

    is_unique = (conn_.use_count() == 1); // not atomic

    // To write or read conn_ is not thread-safe,
    // we can copy it first, for the duplication, 
    // it is thread-safe. 
    // Because conn_ if is not released, conn also.
    // In the following operations, it is also alive
    // since ref count is not 0(i.e. pinned). Otherwise, 
    // conn is released. we can't access it.
    conn = conn_;
  } 
  
  // Has established new connection 
  if (conn) {
    assert(conn->GetLoop() == loop_);
    // Should not use old close callback
    // may be other thread using it
    if (is_unique) {
      conn->ForceClose();
    }
  } else {
    // stop reconnecting
    connector_->Stop();
  }
  
}

void TcpClient::Connect() noexcept {
  connect_ = true;

  connector_->StartRun();
}

void TcpClient::Disconnect() noexcept {
  assert(connect_);
  connect_ = false;

  // FIXME use weak callback?
  MutexGuard guard{ mutex_ };
  if (conn_) {
    conn_->ShutdownWrite();
  }
}

void TcpClient::Stop() noexcept {
  assert(connect_);
  connect_ = false;
  // in loop
  connector_->Stop();
}

