#include "example/file_transfer/client/client.h"

int main(int argc, char* argv[])
{
  if (argc < 3) {
    ::puts("Usage: ./file_transfer_client <absolute-path of file uploaded> <remote path>");
    return 0;
  }

  EventLoopThread ev_thr;
  auto loop = ev_thr.StartRun();
  loop->SetEdgeTriggerMode();

  FileTransferClient client(*loop, InetAddr("127.0.0.1", 9999));

  client.SetConnectionCallback([&](TcpConnectionPtr const& conn) {
    if (conn->IsConnected()) {
      LOG_INFO << "Connected!";
    }
  });

  client.Connect();

  client.SendFile(argv[1], argv[2]);


  LOG_INFO << "Sleep to simulate long-running program";
  ::sleep(10);
  LOG_INFO << "Sleep end";
}