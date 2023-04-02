#include "kanon/net/connection/connection_base.h"

#include "kanon/net/chunk_list.h"

using namespace kanon;

template <typename D>
void ConnectionBase<D>::HandleRead(TimeStamp recv_time)
{
  int saved_errno = 0;
  auto readn = channel_->transferred_bytes;

  input_buffer_.AdvanceWrite(readn);
  if (readn == input_buffer_.GetReadableSize()) {
    input_buffer_.ReserveWriteSpace(readn << 1);
  }
  LOG_TRACE_KANON << "Read " << readn << " bytes from [Connection: " << name_
                  << ", fd = " << channel_->GetFd() << "]";
  if (readn <= 0) {
    LOG_DEBUG_KANON << "Peer close connection";
    HandleClose();
    return;
  }

  if (message_callback_) {
    message_callback_(shared_from_this(), input_buffer_, recv_time);
  } else {
    input_buffer_.AdvanceAll();
    LOG_WARN_KANON << "If user want to process message from peer, should set "
                "proper message_callback_";
  }

  if (input_buffer_.GetWritableSize() == 0) {
    input_buffer_.ReserveWriteSpace(1024);
  }

  BufferOverlapRecv(input_buffer_, channel_->GetFd(), saved_errno, this);
  channel_->EnableReading();
}

template <typename D>
void ConnectionBase<D>::HandleWrite()
{
  loop_->AssertInThread();

  auto writen = channel_->transferred_bytes;
  if (writen <= 0) return;

  LOG_TRACE_KANON << "Write " << writen << " bytes to [Connection: " << name_
                  << ", fd: " << channel_->GetFd() << "]";

  output_buffer_.AdvanceRead(writen);

  if (!output_buffer_.HasReadable()) {
    if (write_complete_callback_) {
      loop_->QueueToLoop(std::bind(
          &ConnectionBase<D>::CallWriteCompleteCallback, shared_from_this()));
    } else {
      channel_->DisableWriting();
    }

    if (state_ == kDisconnecting) {
      ShutdownWrite();
    }
  } else {
    int saved_errno = 0;
    ChunkListOverlapSend(output_buffer_, channel_->GetFd(), saved_errno, this);
    channel_->EnableWriting();
  }
}

template <typename D>
void ConnectionBase<D>::SendInLoop(void const *data, size_t len)
{
  output_buffer_.Append(data, len);
  auto saved_errno = 0;
  ChunkListOverlapSend(output_buffer_, channel_->GetFd(), saved_errno, this);
  channel_->EnableWriting();
}

template <typename D>
void ConnectionBase<D>::SendInLoopForBuf(InputBuffer &buffer)
{
  output_buffer_.Append(buffer.ToStringView());
  auto saved_errno = 0;
  ChunkListOverlapSend(output_buffer_, channel_->GetFd(), saved_errno, this);
  channel_->EnableWriting();
}

template <typename D>
void ConnectionBase<D>::SendInLoopForChunkList(OutputBuffer &buffer)
{
  output_buffer_ = std::move(buffer);
  auto saved_errno = 0;
  ChunkListOverlapSend(output_buffer_, channel_->GetFd(), saved_errno, this);
  channel_->EnableWriting();
}

template <typename D>
void ConnectionBase<D>::HandleReadImmediately(size_t readn)
{
  int saved_errno = 0;

  input_buffer_.AdvanceWrite(readn);
  if (readn == input_buffer_.GetReadableSize()) {
    input_buffer_.ReserveWriteSpace(readn << 1);
  }
  LOG_DEBUG_KANON << "Read " << readn << " bytes from [Connection: " << name_
                  << ", fd = " << channel_->GetFd() << "]";
  if (readn <= 0) {
    LOG_DEBUG_KANON << "Peer close connection";
    HandleClose();
    return;
  }

  if (message_callback_) {
    auto recv_time = TimeStamp::Now();
    message_callback_(shared_from_this(), input_buffer_, recv_time);
    if (input_buffer_.GetWritableSize() == 0) {
      input_buffer_.ReserveWriteSpace(1024);
    }
  } else {
    input_buffer_.AdvanceAll();
    LOG_WARN_KANON << "If user want to process message from peer, should set "
                "proper message_callback_";
  }

  BufferOverlapRecv(input_buffer_, channel_->GetFd(), saved_errno, this);
  channel_->EnableReading();
}

template <typename D>
void ConnectionBase<D>::HandleWriteImmediately(size_t writen)
{
  loop_->AssertInThread();

  output_buffer_.AdvanceRead(writen);

  if (!output_buffer_.HasReadable()) {
    if (write_complete_callback_) {
      loop_->QueueToLoop(std::bind(
          &ConnectionBase<D>::CallWriteCompleteCallback, shared_from_this()));
    } else {
      channel_->DisableWriting();
    }

    if (state_ == kDisconnecting) {
      ShutdownWrite();
    }
  } else {
    int saved_errno = 0;
    ChunkListOverlapSend(output_buffer_, channel_->GetFd(), saved_errno, this);
  }
}
