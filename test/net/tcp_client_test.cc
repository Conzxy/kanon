#include "kanon/net/tcp_client.h"
#include "kanon/net/user_server.h"

using namespace kanon;

EventLoop *g_loop = nullptr;

int main()
{
  EventLoop loop{};
  g_loop = &loop;

  InetAddr serv_addr{"127.0.0.1", 9999};
  auto client = NewTcpClient(g_loop, serv_addr);
  client->Connect();

  client->SetConnectionCallback([](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      // Use socat as a print server
      conn->Send("Hello World!");
    } else {
      LOG_INFO << "Client is down";
    }
  });

  client->SetMessageCallback([](TcpConnectionPtr const &conn, Buffer &buffer, TimeStamp) {
    LOG_INFO << buffer.ToStringView();
    buffer.AdvanceAll();
  });

  loop.StartLoop();
}
