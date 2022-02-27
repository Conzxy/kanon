#include "kanon/net/tcp_server.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/buffer.h"
#include "kanon/net/inet_addr.h"
#include "kanon/log/logger.h"

using namespace kanon;

int main() {
  EventLoop loop;
  
  InetAddr listen_addr{ 9999 };

  TcpServer server(&loop, listen_addr, "TcpServer test");
  
  server.SetMessageCallback([](TcpConnectionPtr const& conn,
                 Buffer& input_buffer,
                 TimeStamp stamp) {
      KANON_UNUSED(stamp);
      KANON_UNUSED(conn);
      LOG_INFO << input_buffer.RetrieveAllAsString();

  });

  server.SetLoopNum(2); 
  server.StartRun();
  loop.StartLoop();
    
}
