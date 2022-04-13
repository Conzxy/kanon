#include "kanon/net/callback.h"
#include "kanon/net/user_server.h"
#include "kanon/util/time_stamp.h"

#include "daytime_format.h"

using namespace kanon;

char const kDaytime[] = "daytime";

class DayTimeServer : public TcpServer {
public:
  DayTimeServer(EventLoop* loop)
    : TcpServer(loop, InetAddr(9999), "daytime")
  {
    SetConnectionCallback(std::bind(&DayTimeServer::OnConnection, this, _1));
  }

  void OnConnection(
    TcpConnectionPtr const& conn)
  {
      conn->SetWriteCompleteCallback([](TcpConnectionPtr const& conn) {
        conn->ShutdownWrite();
        return true;
      });

      conn->Send(GetDaytime());
  }
};

int main()
{
  EventLoop loop;

  DayTimeServer server(&loop);
  
  server.StartRun(); 
  loop.StartLoop();
}
