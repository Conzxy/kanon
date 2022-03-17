#include "kanon/net/tcp_server.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/tcp_connection.h"
#include "kanon/net/sock_api.h"

#include <unistd.h>

using namespace kanon;

int main() {
  EventLoop loop;
  InetAddr listen_addr{ 9999 };

  TcpServer server{ &loop, listen_addr, "SIGPIPE test"};
  
  server.SetConnectionCallback([](TcpConnectionPtr const& conn) {
      if (conn->IsConnected()) {
        ::sleep(4);

        // first write is normal
        // but continue write, it will receive SIGPIPE
        conn->Send("aaaa");
        conn->Send("aaaa");
      }

  });
  
  server.StartRun();

  loop.StartLoop();
}
