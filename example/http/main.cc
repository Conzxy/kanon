#include "http_server.h"

using namespace http;

int main() {
  EventLoop loop;
  HttpServer server(&loop);
  
  server.StartRun();

  loop.StartLoop();
}