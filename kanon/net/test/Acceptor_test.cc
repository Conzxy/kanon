#include "kanon/net/acceptor.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/inet_addr.h"
#include "kanon/net/sock_api.h"

using namespace kanon;

int main() {
  EventLoop loop{};

  InetAddr listen_addr{ 9999 };
  
  Acceptor acceptor(&loop, listen_addr);

  acceptor.SetNewConnectionCallback([](int sockfd, InetAddr const& cli_addr) {
    LOG_INFO << "accept a new connection from " << cli_addr.ToIpPort();
    char const data[] = "How are you?";
    auto n = sizeof data;
    LOG_INFO << "n= " << n;
    do {
      auto x = sock::Write(sockfd, data, sizeof data);
      n -= x;
      LOG_INFO << "x= " << x;
    } while(n != 0);

    sock::Close(sockfd);
  });

  acceptor.Listen();
  LOG_DEBUG << "listen_addr: " << listen_addr.ToIpPort();  

  loop.StartLoop();

}