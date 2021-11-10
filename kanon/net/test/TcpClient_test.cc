#include "kanon/net/TcpClient.h"
#include "kanon/net/common.h"

using namespace kanon;

EventLoop* g_loop = nullptr;

int main() {
  EventLoop loop{ };
  g_loop = &loop;
  
  InetAddr serv_addr{ "127.0.0.1", 9999 };
  TcpClient client{ g_loop, serv_addr };

  client.connect();
  
  // loop.queueToLoop([&]() {
  //   auto connection = client.connection();
  //   assert(connection);

  //   connection->send("client");
  // });

  loop.loop();

}
