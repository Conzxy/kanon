#include "kanon/net/tcp_client.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/inet_addr.h"
#include "kanon/net/connector.h"
#include "kanon/net/tcp_connection.h"
#include "kanon/net/sock_api.h"

using namespace kanon;

TcpClient::TcpClient(
  EventLoop* loop,
  InetAddr const& servAddr,
  std::string const& name)
  : loop_{ loop }
  , connector_{ kanon::make_unique<Connector>(loop, servAddr) }
  , name_{ name }
  , connection_callback_{ &DefaultConnectionCallback }
  , connect_{ true }
  , retry_{ true }
  , mutex_{ }
{
  LOG_INFO << "TcpClient-[" << name_ << "]" << " constructed";

  connector_->SetNewConnectionCallback([this, &servAddr](int sockfd) {
    auto new_conn = std::make_shared<TcpConnection>(
      loop_, 
      name_,
      sockfd,
      sock::GetLocalAddr(sockfd),
      servAddr);

    new_conn->SetMessageCallback(message_callback_);
    new_conn->SetWriteCompleteCallback(write_complete_callback_);
    new_conn->SetConnectionCallback(connection_callback_);
  
    // Since stack is private, if we directly contruct and set new connection,
    // that is dangerous! we may be get a half-completed connection.
    {
      MutexGuard guard{ mutex_ };
      conn_ = new_conn;
    }

    conn_->SetCloseCallback([this](TcpConnectionPtr const& conn) {
      loop_->AssertInThread();
      // passive close connection
      assert(conn == conn_);
      assert(conn->GetLoop() == loop_);
   
      {
        MutexGuard guard{ mutex_ };
        conn_.reset();
      } 
  
      // @warning in event handle phase, don't disable and remove channel 
      loop_->QueueToLoop([&conn] () {
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
  LOG_INFO << "TcpClient-[" << name_ << "]" << "destructed";

  bool is_unique = false;
  TcpConnectionPtr conn;
  {
    MutexGuard guard{ mutex_ };

    is_unique = (conn_.use_count() == 1); // not atomic
    conn = conn_;
  } 
  
  // has established new connection 
  if (conn) {
    assert(conn_->GetLoop() == loop_);
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

void
TcpClient::Connect() noexcept {
  connect_ = true;

  connector_->StartRun();
}

void
TcpClient::Disconnect() noexcept {
  connect_ = false;

  // FIXME use weak callback?
  {
    MutexGuard guard{ mutex_ };
    if (conn_) {
      conn_->ShutdownWrite();
    }
  }
}

void
TcpClient::Stop() noexcept {
  connect_ = false;
  // in loop
  connector_->Stop();
}

