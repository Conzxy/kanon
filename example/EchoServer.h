#ifndef KANON_EXAMPLE_ECHO_SERVER_H
#define KANON_EXAMPLE_ECHO_SERVER_H

#include "kanon/net/common.h"
#include "kanon/net/TcpServer.h"

namespace echo {

using namespace kanon; // used in echo namespace, it is not wrong

class EchoServer : kanon::noncopyable {
public:
  explicit EchoServer(InetAddr const& listen_addr)
    : server_{ &loop_, listen_addr, "Echo Server" }
  { 
    server_.setMessageCallback([](
          kanon::TcpConnectionPtr const& conn,
          kanon::Buffer& buffer,
          kanon::TimeStamp stamp) {
        assert(conn->isConnected());
        
        conn->send(buffer);
        LOG_INFO << "now: " << stamp.toFormattedString(false);
    });

    server_.setWriteCompleteCallback([] (TcpConnectionPtr const& conn) {
        LOG_INFO << "write complete for echo message";
    });
  }
  
  void start() KANON_NOEXCEPT {
    server_.start();

    loop_.loop();
  } 

private:
  EventLoop loop_; 
  TcpServer server_;
};

} // namespace echo

#endif // KANON_EXAMPLE_ECHO_SERVER_H
