#include "chat_client.h"

#include <iostream>

using namespace kanon;

ChatClient::ChatClient(kanon::EventLoop *loop, InetAddr const &server_addr)
  : client_{NewTcpClient(loop, server_addr, "Chat Client")}
  , codec_{[this](TcpConnectionPtr const &conn, Buffer &msg,
                  TimeStamp receive_time) {
    this->OnStringMessage(conn, msg.ToStringView().ToString(), receive_time);
    msg.AdvanceAll();
  }}
  , latch_{1}
{
  client_->SetMessageCallback([this](TcpConnectionPtr const &conn,
                                     Buffer &buffer, TimeStamp receive_time) {
    this->codec_.OnMessage(conn, buffer, receive_time);
  });

  client_->SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    DefaultConnectionCallback(conn);

    if (conn->IsConnected()) {
      conn_ = conn;
      latch_.Countdown();
    } else {
      conn_.reset();
    }
  });

  client_->EnableRetry();
}

void ChatClient::OnStringMessage(TcpConnectionPtr const &conn,
                                 std::string const &msg, TimeStamp receive_time)
{
  latch_.Countdown();
  KANON_UNUSED(conn);
  KANON_UNUSED(receive_time);

  printf("<<< %s\n", msg.c_str());
}

void ChatClient::Start()
{
  std::string line;
  while (true) {
    std::cout << "Please type a message: ";

    if (std::getline(std::cin, line)) {
      if (std::cin.eof()) break;
      if (line == "quit") break;
      WriteMessage(line);
    }
    latch_.Reset(1);
    latch_.Wait();
  }
}

void ChatClient::WriteMessage(StringView msg)
{
  MutexGuard guard{mutex_};

  if (conn_) {
    codec_.Send(conn_, msg);
  }
}

void ChatClient::Connect()
{
  ::puts("conecting...");
  client_->Connect();

  latch_.Wait();
  ::puts("connected!");
}

void ChatClient::Disconnect() { client_->Disconnect(); }
