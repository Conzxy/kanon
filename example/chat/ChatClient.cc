#include "ChatClient.h"

using namespace kanon;

ChatClient::ChatClient(kanon::EventLoop* loop,
    InetAddr const& server_addr) 
  : client_{ loop, server_addr, "Chat Client" }
  , codec_{ [this](TcpConnectionPtr const& conn,
      std::string const& msg,
      TimeStamp receive_time) {
    this->onStringMessage(conn, msg, receive_time);
  } }
  , latch_{ 1 }
{
  client_.setMessageCallback([this](TcpConnectionPtr const& conn,
        Buffer& buffer,
        TimeStamp receive_time) {
      this->codec_.onMessage(conn, buffer, receive_time);
  });
  
  client_.setConnectionCallback([this](TcpConnectionPtr const& conn) {
    defaultConnectionCallaback(conn);

    if (conn->isConnected()) {
      conn_ = conn;
      latch_.countdown();
    } else {
      conn_.reset();
    }
  });

  client_.enableRetry();
}

void
ChatClient::onStringMessage(TcpConnectionPtr const& conn,
    std::string const& msg,
    TimeStamp receive_time) {
  KANON_UNUSED(conn);
  KANON_UNUSED(receive_time);
  
  printf("\n<<< %s\n", msg.c_str());
}

void
ChatClient::writeMessage(StringView msg) {
  MutexGuard guard{ mutex_ };

  if (conn_) {
    codec_.send(conn_, msg);
  }  
}

void
ChatClient::connect() {
  puts("conecting...");
  client_.connect();

  latch_.wait();
  puts("connected!");
}

void
ChatClient::disconnect() {
  client_.disconnect();
}
