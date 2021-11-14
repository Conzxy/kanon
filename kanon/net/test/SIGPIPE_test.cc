#include "kanon/net/TcpServer.h"
#include "kanon/net/EventLoop.h"
#include "kanon/net/TcpConnection.h"
#include "kanon/net/sock_api.h"

#include <unistd.h>

using namespace kanon;

int main() {
  EventLoop loop;
  InetAddr listen_addr{ 9999 };

  TcpServer server{ &loop, listen_addr, "SIGPIPE test"};
  
  server.setConnectionCallback([](TcpConnectionPtr const& conn) {
      if (conn->isConnected()) {
        ::sleep(4);

        // first write is normal
        // but continue write, it will receive SIGPIPE
        conn->send("aaaa");
        conn->send("aaaa");
      }

  });
  
  server.listen();

  loop.loop();
}
