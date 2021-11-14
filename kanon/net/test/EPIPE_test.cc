#include "kanon/net/TcpServer.h"
#include "kanon/net/EventLoop.h"
#include "kanon/net/TcpConnection.h"

#include <unistd.h>

using namespace kanon;

int main() {
  EventLoop loop;
  InetAddr listen_addr{ 9999 };

  TcpServer server{ &loop, listen_addr, "EPIPE test"};
  
  server.setConnectionCallback([](TcpConnectionPtr const& conn) {
      if (conn->isConnected()) {
        ::sleep(4);
        conn->send("a");
      }

  });

}
