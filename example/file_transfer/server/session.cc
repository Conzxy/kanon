#include "session.h"
#include "kanon/net/user_server.h"

using namespace kanon;

FileTransferSession::FileTransferSession(TcpConnection* conn)
  : conn_(conn)
  , codec_(std::bind(&FileTransferSession::OnLengthMessage, this, kanon::_1, kanon::_2, kanon::_3))
  , seq_(0)
{
  LOG_DEBUG << "FileTransferSession contructed";
  codec_.SetMaximumAcceptLength(kMaxAcceptLength);
  conn_->SetMessageCallback(std::bind(
    &kanon::LengthHeaderCodec::OnMessage, std::ref(codec_), kanon::_1, kanon::_2, kanon::_3));
}

FileTransferSession::~FileTransferSession() noexcept {
  LOG_DEBUG << "FileTransferSession destroyed";
}

void FileTransferSession::OnLengthMessage(
  const TcpConnectionPtr &conn, Buffer& buffer, TimeStamp recv_time)
{
  // Operation Sequence number Filename-Length    Filename          File-Size   File contents
  //  1             4              4             Filename-Length       4        File-Size
  auto op = buffer.Read8();

  if (op & (1 << 7)) {
    LOG_TRACE << "This is a UPLOAD operation";

    auto seq = buffer.Read32();
    LOG_DEBUG << "seq = " << seq;

    if (seq != (seq_)) {
      // error handling
      LOG_INFO << "The peer sent sequence number is not correct";
      return ;
    }

    seq_ = seq + 1;

    auto filename_len = buffer.Read32();
    LOG_DEBUG << "Length of filename = " << filename_len;

    if (filename_len > 4096) {
      // error handling
      return ;
    }

    if (!local_path_.empty()) {
      if (buffer.ToStringView().substr(0, filename_len).compare(local_path_) != 0) {
        return ;
      }
    }
    else {
      local_path_ = buffer.ToStringView().substr(0, filename_len).ToString();
      LOG_DEBUG << "Filename = " << local_path_;
    }

    buffer.AdvanceRead(filename_len);

    // Cache it used for OnWriteComplete()
    auto filesize = buffer.Read32();
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

    if (!file_->Write(buffer.GetReadBegin(), filesize)) {
      LOG_SYSERROR << "File write error";
      buffer.AdvanceRead(filesize);
      return ;
    }

    buffer.AdvanceRead(filesize);

    if (op & 0x1) {
      LOG_TRACE << "Reset state";
      Reset();
    }
  }
  else {
    LOG_TRACE << "This is a DOWNLOAD operation";
  }
}

void FileTransferSession::Reset() noexcept
{
  local_path_.clear();
  seq_ = 0;
  file_.reset();
}