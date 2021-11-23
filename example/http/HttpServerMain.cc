#include "HttpServer.h"

#include "kanon/net/common.h"

using namespace http;

int main() {
  EventLoop loop;
  InetAddr listen_addr{ 80 };
  HttpServer server(&loop, listen_addr);
  
  server.start();

  loop.loop();
}
