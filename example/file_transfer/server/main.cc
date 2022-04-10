#include "example/file_transfer/server/server.h"

int main()
{
  EventLoop loop;

  FileTransferServer server(loop);

  server.StartRun();
  loop.StartLoop();
}