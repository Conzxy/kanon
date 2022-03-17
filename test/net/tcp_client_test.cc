#include "kanon/net/tcp_client.h"
#include "kanon/net/user_server.h"

using namespace kanon;

EventLoop* g_loop = nullptr;

int main() {
  EventLoop loop{ };
  g_loop = &loop;
  
  InetAddr serv_addr{ "127.0.0.1", 9999 };
  TcpClient client{ g_loop, serv_addr };

  client.Connect();
  
  // loop.QueueToLoop([&]() {
  //   auto connection = client.GetConnection();
  //   assert(connection);

  //   connection->Send("client");
  // });

  loop.StartLoop();

}
