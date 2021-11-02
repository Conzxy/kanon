#include "kanon/net/TcpClient.h"
#include "kanon/net/EventLoop.h"
#include "kanon/net/InetAddr.h"
#include "kanon/net/Connector.h"
#include "kanon/net/TcpConnection.h"
#include "kanon/net/sock_api.h"

using namespace kanon;

TcpClient::TcpClient(
  EventLoop* loop,
  InetAddr const& servAddr,
  std::string const& name)
  : loop_{ loop }
  , connector_{ kanon::make_unique<Connector>(loop, servAddr) }
  , name_{ name }
  , connection_callback_{ &defaultConnectionCallaback }
  , connect_{ true }
  , retry_{ true }
  , mutex_{ }
{
  LOG_INFO << "TcpClient-[" << name_ << "]" << " constructed";

  connector_->setNewConnectionCallback([this, &servAddr](int sockfd) {
    auto new_conn = std::make_shared<TcpConnection>(
      loop_, 
      name_,
      sockfd,
      sock::getLocalAddr(sockfd),
      servAddr);

    new_conn->setMessageCallback(message_callback_);
    new_conn->setWriteCompleteCallback(write_complete_callback_);
    new_conn->setConnectionCallback(connection_callback_);
  
    // Since stack is private, it we directly contruct and set new connection,
    // that is dangerous! we may be get a half-completed connection.
    {
      MutexGuard guard{ mutex_ };
      conn_ = new_conn;
    }

    conn_->setCloseCallback([this](TcpConnectionPtr const& conn) {
      loop_->assertInThread();
      // passive close connection
      assert(conn == conn_);
      assert(conn->loop() == loop_);
   
      {
        MutexGuard guard{ mutex_ };
        conn_.reset();
      } 
  
      // @warning in event handle phase, don't disable and remove channel 
      loop_->queueToLoop([&conn] () {
        conn->connectionDestroyed();
      });
      // If user does not call disconnect() and
      // support restart when passive close occurred
      if (connect_ && retry_) {
        connector_->restart();
      }
    });
  
    // enable reading
    conn_->connectionEstablished();
  });
} 

TcpClient::~TcpClient() KANON_NOEXCEPT {
  assert(conn_->loop() == loop_);
  
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
    // Should not use old close callback
    // may be other thread using it
    if (is_unique) {
      conn->forceClose();
    }
  } else {
    // stop reconnecting
    connector_->stop();
  }
  
   
}

void
TcpClient::connect() KANON_NOEXCEPT {
  connect_ = true;

  connector_->start();
}

void
TcpClient::disconnect() KANON_NOEXCEPT {
  connect_ = false;

  // FIXME use weak callback?
  {
    MutexGuard guard{ mutex_ };
    if (conn_) {
      conn_->shutdownWrite();
    }
  }
}

void
TcpClient::stop() KANON_NOEXCEPT {
  connect_ = false;
  // in loop
  connector_->stop();
}

