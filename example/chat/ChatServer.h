#ifndef KANON_CHAT_SERVER_H
#define KANON_CHAT_SERVER_H

#include "kanon/net/TcpServer.h"
#include "kanon/net/common.h"
#include "../codec.h"

#include <list>

class ChatServer : kanon::noncopyable {
public:
  explicit ChatServer(kanon::InetAddr const& addr)
    : server_{ &loop_, addr, "Chat Server" }
    , codec_{ [this](TcpConnectionPtr const& conn,
        std::string const& msg,
        TimeStamp receive_time) { 
      this->onStringMessage(conn, msg, receive_time);
    } }
  { 
    server_.setConnectionCallback([this](kanon::TcpConnectionPtr const& conn) {
        this->onConnection(conn);
    });

    server_.setMessageCallback([this](TcpConnectionPtr const& conn,
          Buffer& buf,
          TimeStamp receive_time) {
        codec_.onMessage(conn, buf, receive_time);
    });
  }
    
  void start(); 
  void onConnection(TcpConnectionPtr const& conn);
  void onStringMessage(TcpConnectionPtr const& conn,
      std::string const& msg,
      TimeStamp receive_time);
private:
  kanon::EventLoop loop_;
  kanon::TcpServer server_;
  
  // Since we need erase and push in specified connection 
  std::set<TcpConnectionPtr> conns_; 
  kanon::LengthHeaderCodec codec_; 
};

#endif // KANON_CHAT_SERVER_H
