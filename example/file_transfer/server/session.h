#ifndef KANON_EXAMPLE_FILE_TRNASFER_H
#define KANON_EXAMPLE_FILE_TRNASFER_H

#include "kanon/net/user_server.h"
#include "example/length_codec/codec.h"
#include "example/file_transfer/util/file.h"

class FileTransferSession {
public:
  FileTransferSession(kanon::TcpConnection* conn);
  ~FileTransferSession() noexcept;
  void OnLengthMessage(TcpConnectionPtr const& conn, Buffer& msg, TimeStamp recv_time);
private:
  kanon::TcpConnection* conn_;
  kanon::LengthHeaderCodec codec_;
  std::string local_path_;
  uint32_t seq_;
  std::unique_ptr<util::File> file_; 

  uint32_t ExtractInt32(kanon::StringView& num) noexcept;
  void Reset() noexcept;

  // operation + sequence number + filename-length + filename + file-size + file-content + END(last chunk)
  // 8 + 4 + 4 + 4096 + 4 + 65536 + 3
  static constexpr uint32_t kMaxAcceptLength = 69655;
  static constexpr int kFileBufferSize = 64 * 1024;
};

#endif // KANON_EXAMPLE_FILE_TRANSFER_H