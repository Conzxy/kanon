#include "kanon/net/tcp_server.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/tcp_connection.h"

#include <unistd.h>

using namespace kanon;

int main() {
  EventLoop loop;
  InetAddr listen_addr{ 9999 };

  TcpServer server{ &loop, listen_addr, "EPIPE test"};
  
  server.SetConnectionCallback([](TcpConnectionPtr const& conn) {
      if (conn->IsConnected()) {
        ::sleep(4);
        conn->Send("a");
      }

  });

}
