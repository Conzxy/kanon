#include "echo_server.h"

using namespace kanon;

int main() {
  EventLoop loop;

  EchoServer server(loop);

  server.StartRun();
  loop.StartLoop();
}
