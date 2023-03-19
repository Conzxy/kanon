#include "chat_client.h"

using namespace kanon;

ChatClient::ChatClient(kanon::EventLoop* loop,
                       InetAddr const& server_addr) 
  : TcpClient { loop, server_addr, "Chat Client" }
  , codec_{ [this](TcpConnectionPtr const& conn,
      Buffer& msg,
      TimeStamp receive_time) {
    this->OnStringMessage(conn, msg.ToStringView().ToString(), receive_time);
  } }
  , latch_{ 1 }
{
  SetMessageCallback([this](TcpConnectionPtr const& conn,
        Buffer& buffer,
        TimeStamp receive_time) {
      this->codec_.OnMessage(conn, buffer, receive_time);
  });
  
  SetConnectionCallback([this](TcpConnectionPtr const& conn) {
    DefaultConnectionCallback(conn);

    if (conn->IsConnected()) {
      conn_ = conn;
      latch_.Countdown();
    } else {
      conn_.reset();
    }
  });

  EnableRetry();
}

void
ChatClient::OnStringMessage(TcpConnectionPtr const& conn,
    std::string const& msg,
    TimeStamp receive_time) {
  KANON_UNUSED(conn);
  KANON_UNUSED(receive_time);
  
  printf("\n<<< %s\n", msg.c_str());
}

void ChatClient::WriteMessage(StringView msg) {
  MutexGuard guard{ mutex_ };

  if (conn_) {
    codec_.Send(conn_, msg);
  }  
}

void ChatClient::Connect() {
  ::puts("conecting...");
  TcpClient::Connect();

  latch_.Wait();
  ::puts("connected!");
}

void ChatClient::Disconnect()
{
  TcpClient::Disconnect();
}
