#include "kanon/net/user_server.h"

class EchoServer : public TcpServer {
 public:
  explicit EchoServer(EventLoop &loop)
    : TcpServer(&loop, InetAddr(9998), "Echo Server")
  {
    SetMessageCallback([](kanon::TcpConnectionPtr const &conn,
                          kanon::Buffer &buffer, kanon::TimeStamp) {
      conn->Send(buffer);
      buffer.AdvanceRead(buffer.GetReadableSize());
    });
  }
};

using namespace kanon;

int main()
{
  EventLoop loop;

  EchoServer server(loop);

  server.StartRun();
  loop.StartLoop();
}
