#ifndef KANON_FILE_TRANSFER_CLIENT_H
#define KANON_FILE_TRANSFER_CLIENT_H

#include <unistd.h>
#include <inttypes.h>

#include "kanon/net/user_client.h"
#include "kanon/thread/count_down_latch.h"
#include "kanon/net/sock_api.h"

#include "example/length_codec/codec.h"
#include "example/file_transfer/util/file.h"

class FileTransferClient : public TcpClient {
public:
  FileTransferClient(EventLoop& loop, InetAddr const& server_addr);
  ~FileTransferClient();

  void SendFile(std::string local_path, std::string path);
  bool OnWriteComplete();
  void Connect();
private:
  kanon::LengthHeaderCodec codec_;
  std::unique_ptr<util::File> file_;
  std::string local_path_;
  std::string remote_path_;
  uint32_t seq_;

  kanon::CountDownLatch latch_;
  void Reset();

  static constexpr uint32_t kFileBufferSize = 64 * 1024;
};

#endif //