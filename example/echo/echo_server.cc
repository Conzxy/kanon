#include "echo_server.h"

using namespace kanon;

int main(int argc, char **argv) {
  EventLoop loop;
  
  EchoServer server(loop);
  if (argc > 1) {
    if (strcmp(argv[1], "1") == 0) {
      LOG_INFO << "Pool is enabled";
      server.SetChunkPerBlock(10000);
      server.EnablePool(true);
    }
  }

  server.StartRun();
  loop.StartLoop();
}
