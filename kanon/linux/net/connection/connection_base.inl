#include "kanon/net/connection/connection_base.h"

using namespace kanon;

template <typename D>
void ConnectionBase<D>::HandleRead(TimeStamp recv_time)
{
  loop_->AssertInThread();
  if (loop_->IsEdgeTriggerMode()) {
    HandleEtRead(recv_time);
  } else {
    HandleLtRead(recv_time);
  }
}

template <typename D>
void ConnectionBase<D>::HandleWrite()
{
  loop_->AssertInThread();

  // HandleClose() is called OR server/client is destoryed
  // 1. HandleClose() call DisableAll()
  // 2. ConnectionDestoryed() is called when connection is active
  //    (e.g. TcpServer desctroyed)
  if (!channel_->IsWriting()) {
    assert(state_ == kDisconnected);
    LOG_TRACE_KANON << "This Connection: " << name_ << " is down";
    return;
  }

  if (loop_->IsEdgeTriggerMode()) {
    HandleEtWrite();
  } else {
    HandleLtWrite();
  }
}

template <typename D>
void ConnectionBase<D>::SendInLoopForBuf(InputBuffer &buffer)
{
  LOG_TRACE_KANON << "Connection: [" << name_
                  << "], fd = " << channel_->GetFd();

  if (state_ != kConnected) {
    LOG_WARN << "This connection[" << name_
             << "] is not connected, don't send any message";
    return;
  }

  // if (!channel_->IsWriting() && !output_buffer_.HasReadable()) {
  if (!output_buffer_.HasReadable()) {
    // output_buffer_.swap(buffer);

    // auto n = sock::Write(
    //   channel_->GetFd(),
    //   output_buffer_.ToStringView().data(),
    //   output_buffer_.GetReadableSize());

    output_buffer_.Append(buffer.ToStringView());
    int saved_errno = 0;
    auto n = ChunkListWriteFd(output_buffer_, channel_->GetFd(), saved_errno);

    if (saved_errno && saved_errno != EAGAIN) {
      LOG_SYSERROR << "write unexpected error occurred";
    }

    if (n > 0) {
      LOG_TRACE_KANON << "Write length = " << n;
      output_buffer_.AdvanceRead(n);
      if (output_buffer_.HasReadable()) {
#ifdef PRINT_REMAIN
        LOG_TRACE_KANON << "Remaining length = "
                        << output_buffer_.GetReadableSize();
#endif

        if (output_buffer_.GetReadableSize() >= high_water_mark_ &&
            high_water_mark_callback_)
        {
          loop_->QueueToLoop(std::bind(high_water_mark_callback_,
                                       this->shared_from_this(),
                                       output_buffer_.GetReadableSize()));
        }
        channel_->EnableWriting();
      } else {
        if (write_complete_callback_) {
          loop_->QueueToLoop(
              std::bind(write_complete_callback_, this->shared_from_this()));
        }

        if (channel_->IsWriting()) {
          channel_->DisableWriting();
        }
      }
    }
  } else {
    SendInLoop(buffer.ToStringView());
  }
}

template <typename D>
void ConnectionBase<D>::SendInLoopForChunkList(OutputBuffer &buffer)
{
  KANON_ASSERT(
      !output_buffer_.HasReadable(),
      "The Send() for ChunkList must be called when output_buffer_ is empty");

  int saved_errno = 0;
  auto write_n = ChunkListWriteFd(buffer, channel_->GetFd(), saved_errno);

  if (saved_errno && saved_errno != EAGAIN) {
    LOG_SYSERROR << "write unexpected error occurred";
  }

  if (write_n > 0) {
    LOG_DEBUG_KANON << "Write " << write_n << " bytes";
    buffer.AdvanceRead(write_n);

    if (buffer.HasReadable()) {
      if (high_water_mark_callback_ &&
          buffer.GetReadableSize() > high_water_mark_)
      {
        loop_->QueueToLoop(std::bind(high_water_mark_callback_,
                                     this->shared_from_this(),
                                     high_water_mark_));
      }

      output_buffer_.swap(buffer);
    } else {
      LOG_DEBUG_KANON << "Write complete";
      if (write_complete_callback_) {
        loop_->QueueToLoop(std::bind(&ConnectionBase::CallWriteCompleteCallback,
                                     this->shared_from_this()));
      }

      if (channel_->IsWriting()) {
        channel_->DisableWriting();
      }
    }
  }
}

template <typename D>
void ConnectionBase<D>::SendInLoop(void const *data, size_t len)
{
  ssize_t n = 0;
  size_t remaining = len;

  LOG_TRACE_KANON << "Connection: [" << name_
                  << "], fd = " << channel_->GetFd();
  // Although Send() has checked state_ is kConnected
  // But connection also can be closed in the phase 2
  // when this is called in phase 3
  if (state_ != kConnected) {
    LOG_WARN << "This connection [" << name_
             << "] is not connected, don't send any message";
    return;
  }

  //=============DELETED==================//
  //// If is not writing state, indicates output_buffer_ maybe is empty,
  //// but also output_buffer_ is filled by user throught GetOutputBuffer().
  //// When it is not writing state and output_buffer_ is empty,
  //// we can write directly first
  //=============DELETED==================//

  // Although channel in the writing state, we can write directly when the
  // output_buffer_ is empty.
  //
  // The reason for why I strict to the writing is that I want to support
  // pipeline writing and don't disable writing when write complete not
  // actually after HandleWrite() complete.
  // If write complete is not of the last chunk, then disable writing
  // and might register again later. This is wasteful. Because EnableWriting()
  // and DisableWriting() both is forwarded to call ::epoll_ctl() that is
  // expensive.
  // So, My solution is:
  // In HandleWrite(), if write complete but write_complete_callback_ return
  // false, I don't disable writing(If it is empty callback, think it return
  // true). Disable writing only when it return true. In SendInLoop(),
  // regardless of write_complete_callback_, if it is writing state after write
  // complete, disable writing state since next write must call write directly
  // instead of HandleWrite().
  //
  // In short, I want to achieve:
  // Send() --> EnableWriting()[incomplete] --> HandleWrite()[complete, but
  // don't disable]
  // --> Send()[write_complete_callback_, incomplete] -->
  // HandleWrite()[complete, but don't disable]
  // --> Send()[write_complete_callback_, incomplete] -->
  // HandleWrite()[complete, last chunk, disable]
  // --> DisableWriting()
  // Compared with the original process:
  // Send() --> EnableWriting()[incomplete] --> HandleWrite()[complete] -->
  // DisableWriting()
  // --> Send()[write_complete_callback_, incomplete] --> EnableWriting() -->
  // HandleWrite()[complete]
  // --> DisableWriting() --> Send()[write_complete_callback_] -->
  // EnableWriting() --> HandleWrite()[complete] --> DisableWriting()
  //
  // Also, might:
  // Send() --> EnableWriting()[incomplete] --> HandleWrite()[complete, but
  // don't disable]
  // --> Send()[write_complete_callback_, incomplete] -->
  // HandleWrite()[complete, but don't disable]
  // --> Send()[write_complete_callback_, complete but not last chunk] -->
  // DisableWriting()
  // --> Send()[write_complete_callback_, incomplete] --> EnableWriting() -->
  // ...
  //
  // Compared with the original process:
  // Send() --> EnableWriting()[incomplete] --> HandleWrite()[complete] -->
  // DisableWriting()
  // --> Send()[write_complete_callback_, incomplete] --> EnableWriting() -->
  // HandleWrite()[complete]
  // --> DisableWriting() --> Send()[write_complete_callback_, complete but not
  // last chunk]
  // --> Send()[write_complete_callback_, incomplete] --> EnableWriting() -->
  // ...
  //
  // Above two example indicates this approach decrease the number of the call
  // of DisableWriting() and EnableWriting()

  // if (channel_->IsWriting() && output_buffer_.GetReadableSize() == 0) {
  if (!output_buffer_.HasReadable()) {
    n = sock::Write(channel_->GetFd(), data, len);

    if (n >= 0) {
      LOG_TRACE_KANON << "Write " << n << " bytes";
      if (static_cast<size_t>(n) != len) {
        remaining -= n;
      } else {
        if (write_complete_callback_) {
          loop_->QueueToLoop(
              std::bind(write_complete_callback_, this->shared_from_this()));
        }

        if (channel_->IsWriting()) {
          LOG_TRACE_KANON << "Write complete but in writing";
          channel_->DisableWriting();
        }

        return;
      }
    } else {
      if (errno != EAGAIN) // EWOULDBLOCK
        LOG_SYSERROR << "write unexpected error occurred";
      return;
    }
  }

  if (remaining > 0) {
    // short write happened
    // Store remaing message to output_buffer_
    // then write callback will handle

    if (high_water_mark_callback_) {
      auto readable_len = output_buffer_.GetReadableSize();

      if (readable_len + remaining >= high_water_mark_ &&
          readable_len < high_water_mark_)
      {
        loop_->QueueToLoop(std::bind(high_water_mark_callback_,
                                     this->shared_from_this(),
                                     readable_len + remaining));
        // loop_->QueueToLoop([this, readable_len, remaining]() {
        //   high_water_mark_callback_(this->shared_from_this(), readable_len +
        //   remaining);
        // });
      }
    }

    LOG_TRACE_KANON << "Remaining content length = " << remaining;
    output_buffer_.Append(static_cast<char const *>(data) + n, remaining);
    if (!channel_->IsWriting()) {
      channel_->EnableWriting();
    }
  }
}
