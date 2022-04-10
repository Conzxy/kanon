#include "example/file_transfer/server/server.h"

#include "kanon/net/sock_api.h"

FileTransferServer::FileTransferServer(EventLoop& loop)
  : kanon::TcpServer(&loop, InetAddr(9999), "FileTransferServer")
  , codec_(std::bind(&FileTransferServer::OnLengthMessage, this, kanon::_1, kanon::_2, kanon::_3))
  , seq_(0)
{
  codec_.SetMaximumAcceptLength(kMaxAcceptLength);
  SetMessageCallback(std::bind(
    &kanon::LengthHeaderCodec::OnMessage, std::ref(codec_), kanon::_1, kanon::_2, kanon::_3));
}

FileTransferServer::~FileTransferServer() = default;

void FileTransferServer::OnLengthMessage(
  const TcpConnectionPtr &conn, std::vector<char>& msg, TimeStamp recv_time)
{
  StringView view(msg.data(), msg.size());

  // UPLOAD Sequence number Filename-Length Filename          File-Size   File contents
  //    6           4              4        Filename-Length       4        File-Size
  if (view.starts_with("UPLOAD")) {
    LOG_TRACE << "This is UPLOAD operation";
    view.remove_prefix(6);

    auto seq = ExtractInt32(view);
    LOG_DEBUG << "seq = " << seq;

    if (seq != (seq_)) {
      // error handling
      LOG_INFO << "The peer sent sequence number is not correct";
      return ;
    }

    seq_ = seq + 1;

    auto filename_len = ExtractInt32(view);
    LOG_DEBUG << "Length of filename = " << filename_len;

    if (filename_len > 4096) {
      // error handling
      return ;
    }

    if (!local_path_.empty()) {
      if (view.substr(0, filename_len).compare(local_path_) != 0) {
        return ;
      }
    }
    else {
      local_path_ = view.substr(0, filename_len).ToString();
      LOG_DEBUG << "Filename = " << local_path_;
    }

    view.remove_prefix(filename_len);

    // Cache it used for OnWriteComplete()
    auto filesize = ExtractInt32(view);
    LOG_DEBUG << "Filesize = " << filesize;

    if (filesize > kFileBufferSize) {
      // error handling
      LOG_INFO << "The peer send file chunk over 64k";
      return ;
    }

    if (!file_) {
      try {
        file_.reset(new util::File(local_path_, util::File::kTruncate));
      } catch (util::FileException const& ex) {
        // error handling
        LOG_ERROR << ex.what();
        return ;
      }
    }

    if (!file_->Write(view.data(), filesize)) {
      LOG_SYSERROR << "File write error";
      return ;
    }

    view.remove_prefix(filesize);

    if (!view.empty()) {
      if (view.size() == 3 && view == "END") {
        view.remove_prefix(3);
        LOG_TRACE << "Reset state";
        Reset();
      }
    }
  }
  else if (view.starts_with("DOWNLOAD")) {
    LOG_TRACE << "This is a DOWNLOAD operation";
  }
}

uint32_t FileTransferServer::ExtractInt32(kanon::StringView& num) noexcept
{
  char buf[4];

  ::memcpy(buf, num.data(), 4);

  // auto n = reinterpret_cast<uint32_t*>(buf);
  uint32_t n;
  ::memcpy(&n, buf, 4);

  num.remove_prefix(4);
  return kanon::sock::ToHostByteOrder32(n);
}

void FileTransferServer::Reset() noexcept
{
  local_path_.clear();
  seq_ = 0;
  file_.reset();
}