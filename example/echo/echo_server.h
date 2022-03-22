#ifndef KANON_EXAMPLE_ECHO_SERVER_H
#define KANON_EXAMPLE_ECHO_SERVER_H

#include "kanon/net/user_server.h"

class EchoServer : public TcpServer {
public:
  explicit EchoServer(EventLoop& loop)
    : TcpServer(&loop, InetAddr(9999), "Echo Server")
  { 
    SetMessageCallback(
      [](kanon::TcpConnectionPtr const& conn,
         kanon::Buffer& buffer,
         kanon::TimeStamp stamp) 
      {
        conn->Send(buffer);
        buffer.AdvanceRead(buffer.GetReadableSize());
      });
  }
  
};

#endif // KANON_EXAMPLE_ECHO_SERVER_H
