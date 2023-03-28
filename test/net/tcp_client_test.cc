#include "kanon/net/tcp_client.h"
#include "kanon/net/user_server.h"

using namespace kanon;

EventLoop *g_loop = nullptr;

int main()
{
  KanonNetInitialize();
  kanon::Logger::SetLogLevel(Logger::KANON_LL_TRACE);
  EventLoop loop{};
  g_loop = &loop;

  InetAddr serv_addr{"47.99.92.230", 9999};
  auto client = NewTcpClient(g_loop, serv_addr, "TcpClient");
  client->Connect();

  client->SetConnectionCallback([](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      // Use socat as a print server
      conn->Send("Hello World!");
    } else {
      LOG_INFO << "Client is down";
    }
  });

  client->SetMessageCallback(
      [](TcpConnectionPtr const &conn, Buffer &buffer, TimeStamp) {
        LOG_INFO << buffer.ToStringView();
        buffer.AdvanceAll();
      });

  loop.StartLoop();
}
