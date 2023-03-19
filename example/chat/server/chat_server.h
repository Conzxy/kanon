#ifndef KANON_CHAT_SERVER_H
#define KANON_CHAT_SERVER_H

#include "kanon/net/user_server.h"
#include "example/length_codec/codec.h"

#include <list>

class ChatServer : public kanon::TcpServer {
public:
  explicit ChatServer(EventLoop& loop)
    : TcpServer{ &loop, InetAddr(9999), "Chat Server" }
    , codec_{ [this](TcpConnectionPtr const& conn,
                     Buffer &msg,
                     TimeStamp receive_time) { 
      this->OnStringMessage(conn, msg.RetrieveAllAsString(), receive_time);
    } }
  { 
    SetConnectionCallback([this](kanon::TcpConnectionPtr const& conn) {
        this->OnConnection(conn);
    });

    SetMessageCallback([this](TcpConnectionPtr const& conn,
                              Buffer& buf,
                              TimeStamp receive_time) {
        codec_.OnMessage(conn, buf, receive_time);
    });
  }
    
  //void StartRun(); 

  void OnConnection(TcpConnectionPtr const& conn);
  void OnStringMessage(TcpConnectionPtr const& conn,
                       std::string const& msg,
                       TimeStamp receive_time);
private: 
  // Since we need erase and push in specified connection 
  std::set<TcpConnectionPtr> conns_; 
  kanon::LengthHeaderCodec codec_; 
};

#endif // KANON_CHAT_SERVER_H
