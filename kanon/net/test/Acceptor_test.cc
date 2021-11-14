#include "kanon/net/Acceptor.h"
#include "kanon/net/EventLoopThread.h"

using namespace kanon;

int main() {
  EventLoopThread loop_thread;

  InetAddr listen_addr{ 9999 };
  
  LOG_DEBUG << "listen_addr: " << listen_addr.toIpPort();  
  auto loop = loop_thread.start();

  Acceptor acceptor(loop, listen_addr);

  acceptor.setNewConnectionCallback([](int sockfd, InetAddr const& cli_addr) {
    LOG_INFO << "accept a new connection from " << cli_addr.toIpPort();
    char const data[] = "How are you?";
    auto n = sizeof data;
    LOG_INFO << "n= " << n;
    do {
      auto x = sock::write(sockfd, data, sizeof data);
      n -= x;
      LOG_INFO << "x= " << x;
    } while(n != 0);

    ::close(sockfd);
  });

  acceptor.listen();

}
