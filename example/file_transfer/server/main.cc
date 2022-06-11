#include "example/file_transfer/server/server.h"

int main(int argc, char* argv[])
{
  if (argc < 2) {
    puts("Usage: ./file_transfer_server [port]");
    return 0;
  }

  EventLoop loop;

  FileTransferServer server(loop, ::atoi(argv[1]));

  server.StartRun();
  loop.StartLoop();
}