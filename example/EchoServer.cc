#include "example/EchoServer.h"

using namespace echo;

int main() {
  InetAddr listen_addr{ 9999 };

  EchoServer server{ listen_addr };

  server.start();
}
