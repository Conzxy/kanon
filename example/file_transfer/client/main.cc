#include "example/file_transfer/client/client.h"

#include <tuple>

using namespace std;

std::tuple<std::string, std::string> SetParams(int argc, char* argv[]) {
  std::string local_path = argv[1];
  std::string remote_path = argv[2];

  assert(remote_path.size() >= 1 && remote_path[0] == '/');

  if (remote_path.back() == '/') {
    auto pos = local_path.rfind('/');

    if (pos == std::string::npos) {
      remote_path += local_path;
    } else {
      remote_path.append(local_path.data()+pos+1, local_path.size()-pos-1);
    }
  }

  return std::make_tuple(local_path, remote_path);
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    ::puts("Usage: ./file_transfer_client <local file path> <remote directory path> [port]");
    return 0;
  }

  EventLoopThread ev_thr;
  auto loop = ev_thr.StartRun();

  FileTransferClient client(*loop, InetAddr("127.0.0.1", (argc >= 4) ? ::atoi(argv[3]) : 9999));

  client.Connect();
  loop->SetEdgeTriggerMode();

  string local_path;
  string remote_path;

  std::tie(local_path, remote_path) = SetParams(argc, argv);

  LOG_DEBUG << "local_path = " << local_path << ", " << "remote_path = " << remote_path;
  client.SendFile(local_path, remote_path);

  LOG_INFO << "Sleep to simulate long-running program";
  ::sleep(10);
  LOG_INFO << "Sleep end";
}