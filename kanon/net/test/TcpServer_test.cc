#include "kanon/net/TcpServer.h"
#include "kanon/net/EventLoop.h"
#include "kanon/net/Buffer.h"

using namespace kanon;

int main() {
  EventLoop loop;
  
  InetAddr listen_addr{ 9999 };

  TcpServer server(&loop, listen_addr, "TcpServer test");
  
  server.setMessageCallback([](TcpConnectionPtr const& conn,
                 Buffer* input_buffer,
                 TimeStamp stamp) {
      KANON_UNUSED(stamp);
      KANON_UNUSED(conn);
      LOG_INFO << input_buffer->retrieveAllAsString();

  });
  
  server.listen();
  loop.loop();
    
}
