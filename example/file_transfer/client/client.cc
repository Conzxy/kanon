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

  const auto flash_pos = local_path_.rfind('/');

  std::string filename;

  // if local_path_: */xxxxx, extract xxxxx
  if (flash_pos != std::string::npos) {
    filename = local_path_.substr(flash_pos+1);
  }
  else {
    filename = local_path_;
  }

  local_path_.reserve(local_path_.size() + filename.size() + 1);

  // if remote_path_ is not end with /
  // add it, e.g. . ==> ./
  if (remote_path_.back() != '/') {
    remote_path_ += '/';
  }

  remote_path_ += filename;

  try {
    file_.reset(new File(local_path_));
  } catch (FileException const& ex) {
    ::puts("This is not a valid filename");
    ::fflush(stdout);
    ::exit(0);
  }

  OnWriteComplete();
}

void Append32(std::string& str, uint32_t n)
{
  char buf[4];
  uint32_t net_n = sock::ToNetworkByteOrder32(n);

  ::memcpy(buf, &net_n, 4);

  str.append(buf, 4);
}

bool FileTransferClient::OnWriteComplete()
{
  auto conn = GetConnection();
  bool ret = true;

  char buf[kFileBufferSize];

  auto n = file_->Read(buf, sizeof buf);

  std::string msg;
  if (n != File::kInvalidReturn) {
    msg.reserve(6 + remote_path_.size() + sizeof seq_ + sizeof n + n);

    // UPLOAD Sequence number Filename-Length Filename          File-Size   File contents
    //    4           4              4        Filename-Length       4        File-Size
    // The last chunk should put "END" in the end
    msg += "UPLOAD";

    LOG_DEBUG << "seq_ = " << seq_;
    Append32(msg, seq_);
    ++seq_;

    Append32(msg, remote_path_.size());
    LOG_DEBUG << "filename length = " << remote_path_.size();

    msg += remote_path_;
    LOG_DEBUG << "remote_path_ = " << remote_path_;

    Append32(msg, n);
    LOG_DEBUG << "filesize = " << n;

    // msg += buf;
    // Binary data
    msg.append(buf, n);

    // 1. char*/StringView
    // 2. Buffer

    if (n < sizeof buf) {
      conn->SetWriteCompleteCallback(WriteCompleteCallback());
      msg += "END";
      codec_.Send(conn, msg);
      Reset();

      Disconnect();
    }
    else {
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