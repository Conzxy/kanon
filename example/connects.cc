#include "kanon/net/InetAddr.h"
#include "kanon/net/sock_api.h"
#include <unistd.h>

#include <vector>

using namespace kanon;

void incConnection(int& conn) {
  conn++;
  printf("established connection number: %d\n", conn);
}

int main(int argc, char** argv) {
  InetAddr servAddr{ "127.0.0.1", 9999 };
  
  if (argc < 2) {
    printf("usage: %s max connection number \n", argv[0]);
    exit(1);
  } 

  int maxConnectionNum = atoi(argv[1]);  
  
  printf("maxConnectionNum: %d\n", maxConnectionNum);
  
  int conn_num = 0;
  std::vector<int> fds;
  fds.reserve(maxConnectionNum);
  
  for (int i = 0; i < maxConnectionNum; ++i) {
    fds.emplace_back(sock::createSocket(0));
  
       sock::connect(fds[i], sock::to_sockaddr(servAddr.toIpv4()));
       incConnection(conn_num);
 }

  for (auto fd : fds)
    ::close(fd);

}
