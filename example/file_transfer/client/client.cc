#include "example/file_transfer/client/client.h"

using namespace kanon;
using namespace util;

FileTransferClient::FileTransferClient(EventLoop& loop, InetAddr const& server_addr)
  : TcpClient(&loop, server_addr, "FileTrasnfer Client")
  , codec_()
  , seq_(0)
  , latch_(1)
{
}

FileTransferClient::~FileTransferClient() = default;

void FileTransferClient::SendFile(std::string local_path, std::string path)
{
  // Cache
  local_path_ = std::move(local_path);
  remote_path_ = std::move(path);

  try {
    file_.reset(new File(local_path_));
  } catch (FileException const& ex) {
    ::perror("This is not a valid filename");
    ::exit(0);
  }

  OnWriteComplete();
}

bool FileTransferClient::OnWriteComplete()
{
  auto conn = GetConnection();
  bool ret = true;

  char buf[kFileBufferSize];

  auto n = file_->Read(buf, sizeof buf);

  OutputBuffer msg;
  if (n != File::kInvalidReturn) {
    uint8_t op = 0;

    op |= (1 << 7);
    if (n < sizeof buf) {
      op |= 1;
    }

    msg.Append8(op);

    LOG_DEBUG << "seq_ = " << seq_;
    msg.Append32(seq_);
    ++seq_;

    msg.Append32(remote_path_.size());
    LOG_DEBUG << "filename length = " << remote_path_.size();

    msg.Append(remote_path_);
    LOG_DEBUG << "remote_path_ = " << remote_path_;

    msg.Append32(n);
    LOG_DEBUG << "filesize = " << n;

    // msg += buf;
    // Binary data
    msg.Append(buf, n);

    // 1. char*/StringView
    // 2. Buffer
    if (n < sizeof buf) {
      conn->SetWriteCompleteCallback(WriteCompleteCallback());
      codec_.Send(conn, msg);
      Reset();
      Disconnect();
    } else {
      conn->SetWriteCompleteCallback(std::bind(
        &FileTransferClient::OnWriteComplete,
        this));
      ret = false;
      codec_.Send(conn, msg);
    }
  }

  return ret;
}

void FileTransferClient::Connect()
{
  LOG_INFO << "Connecting...";

  SetConnectionCallback([this](TcpConnectionPtr const& conn) {
    if (conn->IsConnected()) {
      latch_.Countdown();
      LOG_INFO << "Connected";
    } else {
      conn->SetWriteCompleteCallback({});
    }
  });

  TcpClient::Connect();

  latch_.Wait();
}

void FileTransferClient::Reset()
{
  seq_ = 0;
  file_.reset();
}