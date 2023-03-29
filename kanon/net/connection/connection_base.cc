#include "connection_base.h"

#include <errno.h>
#include <signal.h> // SIGPIPE, signal()
#include <atomic>

#include "kanon/log/logger.h"
#include "kanon/net/socket.h"
#include "kanon/net/channel.h"
#include "kanon/net/event_loop.h"

using namespace std;
using namespace kanon;

static constexpr int kDefaultHighWatermark = 64 * 1024;

#define PRINT_REMAIN

template <typename D>
ConnectionBase<D>::ConnectionBase(EventLoop *loop, std::string const &name,
                                  int sockfd)
  : loop_(loop)
  , name_(name)
  , socket_(new Socket(sockfd))
  , channel_(new Channel(loop_, sockfd))
  , high_water_mark_{kDefaultHighWatermark}
  , state_{kConnecting}
{
  // Pass raw pointer is safe here since
  // will disable all events when connection
  // become disconnectioned(later, it will
  // be destroyed)
  channel_->SetReadCallback(std::bind(&ConnectionBase::HandleRead, this, _1));
  channel_->SetWriteCallback(std::bind(&ConnectionBase::HandleWrite, this));
  channel_->SetErrorCallback(std::bind(&ConnectionBase::HandleError, this));
  channel_->SetCloseCallback(std::bind(&ConnectionBase::HandleClose, this));
}

template <typename D>
ConnectionBase<D>::~ConnectionBase()
{
  assert(state_ == kDisconnected);
  LOG_TRACE_KANON << "~ConnectionBase()";
}

template <typename D>
void ConnectionBase<D>::ConnectionEstablished()
{
  loop_->AssertInThread();

  assert(state_ == kConnecting);
  state_ = kConnected;

  // Start observing read event on this socket
  // and call the OnConnection callback which
  // is set by user or default.
  channel_->EnableReading();
  int saved_errno = 0;
  LOG_DEBUG << "Fd = " << channel_->GetFd();
  kanon::BufferOverlapRecv(input_buffer_, channel_->GetFd(), saved_errno, this);
  LOG_TRACE_KANON << "Connection [" << name_ << "] is established";

  assert(connection_callback_);
  connection_callback_(this->shared_from_this());
}

template <typename D>
void ConnectionBase<D>::ConnectionDestroyed()
{
  // ! Must be called in phase 3(QueueToLoop())
  loop_->AssertInThread();

  LOG_TRACE_KANON << "Connection [" << name_ << "]"
                  << " destoryed";
  // This may be called by TcpServer dtor or close_callback_(see TcpServer)
  // if close_callback_ has be called, just remove channel;
  if (state_ == kConnected) {
    channel_->DisableAll();
    state_ = kDisconnected;

    LOG_TRACE_KANON << "Connection [" << name_ << "] has destroyed";
    // Because ConnectionDestroyed maybe async call
    // Don't pass raw pointer
    connection_callback_(this->shared_from_this());
  }

  assert(state_ == kDisconnected);

  channel_->Remove();
}

template <typename D>
void ConnectionBase<D>::HandleLtRead(TimeStamp recv_time)
{
  /**
   * 1. Call BufferReadFromFd() to get data
   * 2. Check return value
   * 2.1. If 0, indicates peer close connection, call close_callback_
   * 2.2. >0, call message_callback_
   * 2.3. <0, error occurred, call HandleError()
   */
  int saved_errno = 0;
  auto n = BufferReadFromFd(input_buffer_, channel_->GetFd(), saved_errno);

  if (saved_errno != 0) {
    errno = saved_errno;

    if (errno != EAGAIN) {
      LOG_SYSERROR << "Read event handle error";
      HandleError();
    }
  } else if (n == 0) {
    // Peer close the connection
    // Note: Don't distinguish the shutdown(WR) and close()
    LOG_TRACE_KANON << "Peer close connection";
    HandleClose();
  } else {
    assert(n > 0 && n != static_cast<size_t>(-1));

    LOG_DEBUG_KANON << "Read " << n << " bytes from [Connection: " << name_
                    << ", fd: " << channel_->GetFd() << "]";

    if (message_callback_) {
      message_callback_(this->shared_from_this(), input_buffer_, recv_time);
    } else {
      input_buffer_.AdvanceAll();
      LOG_WARN << "If user want to process message from peer, should set "
                  "proper message_callback_";
    }
  }
}

template <typename D>
void ConnectionBase<D>::HandleEtRead(TimeStamp recv_time)
{
  // The event occurred only in the:
  // 1. new message coming
  // 2. buffer zero to low-watermark

  // 1. Call input_buffer_.BufferReadFromFd() get some message
  // 2.1. If the saved_errno is less than 0, and isn't EAGAIN(OR EWOULDBLOCK),
  //    continue read until the errno = EAGAIN
  // 2.2. If the saved_errno is not EAGAIN, it indicates there is a error
  // occurred
  // 3. The message length is greater than 0, call the message_callback_ which
  // is user-defined
  //    If not a valid callback(i.e. empty callback) is also ok, just discard
  //    the contents in input_buffer_
  // 4. The message length is equal to 0, it dicates that peer close this
  // connection
  //    then call HandleClose() since this is passive close
  int saved_errno = 0;

  for (;;) {
    auto readn =
        BufferReadFromFd(input_buffer_, channel_->GetFd(), saved_errno);

    if (saved_errno != 0) {
      if (saved_errno != EAGAIN) {
        LOG_SYSERROR << "Read event handle error";
        HandleError();
      } else {
        break;
      }
    }

    LOG_DEBUG_KANON << "Read " << readn << " bytes from [Connection: " << name_
                    << ", fd: " << channel_->GetFd() << "]";
    if (readn == 0) {
      LOG_DEBUG_KANON << "Peer close connection";
      HandleClose();
      break;
    }

    // FIXME
    // message_callback_ should be called here?
    if (message_callback_) {
      message_callback_(this->shared_from_this(), input_buffer_, recv_time);
    } else {
      input_buffer_.AdvanceAll();
      LOG_WARN << "If user want to process message from peer, should set "
                  "proper message_callback_";
    }
  }
}

template <typename D>
void ConnectionBase<D>::HandleLtWrite()
{
  /**
   * Logical is simple, just write the contents in ouput_buffer_
   * Normally, this be called when
   * 1. Short write
   * 2. Actively write(e.g. output_buffer_ provided by user)
   */

  // The first write don't get EAGAIN
  // If the first write don't write entire message, the second will return
  // EAGAIN Otherwise, there is no need to call the second call of write(). So,
  // just call write once is enough auto n = sock::Write(channel_->GetFd(),
  //           output_buffer_.GetReadBegin(),
  //           output_buffer_.GetReadableSize());
  int saved_errno = 0;
  auto n = ChunkListWriteFd(output_buffer_, channel_->GetFd(), saved_errno);

  LOG_TRACE_KANON << "Write " << n << " bytes to [Connection: " << name_
                  << ", fd: " << channel_->GetFd() << "]";

  if (saved_errno && saved_errno != EAGAIN) {
    LOG_SYSERROR << "Write event handle error";
    HandleError();
  }

  // If saved_errno != 0, indicates there is a error occured.
  // But the returned value of WriteFd() also can be nonnegative
  // since WriteFd() may call ::writev() many times
  if (n > 0) {
    output_buffer_.AdvanceRead(n);
#ifdef PRINT_REMAIN
    LOG_TRACE_KANON << "Output Buffer remaining = "
                    << output_buffer_.GetReadableSize();
#endif
    if (!output_buffer_.HasReadable()) {
      if (write_complete_callback_) {
        // We delay the callback to phase 3
        // to increase the response rate since it
        // is not necessary.
        // For example, suppose this is file write
        // in pileline approach, and short write,
        // the write_complete_callback_ will write
        // continue but also might still don't write
        // completly, but poll phase has passed,
        // The next write must be put in the next loop,
        // So the callback can be delayed to phase 3 is
        // also ok.
        // loop_->QueueToLoop(std::bind(
        //   write_complete_callback_,
        //   this->shared_from_this()));

        // Pass this is unsafe even thought this is in the
        // phase of processing write event since there is a
        // HandleClose() callback has register early
        loop_->QueueToLoop(std::bind(
            &ConnectionBase<D>::CallWriteCompleteCallback, this->shared_from_this()));
      } else {
        channel_->DisableWriting();
      }

      // Disconnecting is set in ShutdownWrite(). Because there are some message
      // need write(channel_->IsWriting()) to kernel space, delay the shutdown()
      // to here
      if (state_ == kDisconnecting) {
        ShutdownWrite();
      }
    }
  }
}

template <typename D>
void ConnectionBase<D>::CallWriteCompleteCallback()
{
  if (write_complete_callback_(this->shared_from_this())) {
    LOG_TRACE_KANON << "Last chunk in the pipeline write";
    // The write_complete_callback_ maybe disable writing in the SendInLoop()
    if (channel_->IsWriting()) {
      channel_->DisableWriting();
    }
  } else {
    LOG_TRACE_KANON
        << "Not last chunk int the pipeline wirte()[don't disable wirting]";
  }
}

template <typename D>
void ConnectionBase<D>::HandleEtWrite()
{
  // The event occurred only in the:
  // 1. buffer high-watermark to low-watermark(writable)

  while (true) {
    // auto writen = sock::Write(
    // channel_->GetFd(),
    // output_buffer_.GetReadBegin(),
    // output_buffer_.GetReadableSize());

    int saved_errno = 0;
    auto writen =
        ChunkListWriteFd(output_buffer_, channel_->GetFd(), saved_errno);

    if (saved_errno) {
      if (saved_errno != EAGAIN) {
        LOG_SYSERROR << "Write event handle error";
        HandleError();
      }
    }

    LOG_TRACE_KANON << "Write " << writen << " bytes to [Connection: " << name_
                    << ", fd: " << channel_->GetFd() << "]";

    output_buffer_.AdvanceRead(writen);

    if (!output_buffer_.HasReadable() && saved_errno == EAGAIN) {
      break;
    }
  }

  if (output_buffer_.HasReadable()) {
    // LOG_TRACE_KANON << "Output Buffer remaining = " <<
    // output_buffer_.GetReadableSize(); To write entire message, we need
    // resigter write event again
    channel_->EnableWriting();
  } else {
    if (write_complete_callback_) {
      // No need to disable writing
      loop_->QueueToLoop(std::bind(
          &ConnectionBase<D>::CallWriteCompleteCallback, this->shared_from_this()));
    }

    if (state_ == kDisconnecting) {
      socket_->ShutdownWrite();
    }
  }
}

template <typename D>
void ConnectionBase<D>::HandleError()
{
  loop_->AssertInThread();
  int err = sock::GetSocketError(channel_->GetFd());
  LOG_SYSERROR << "ConnectionBase<D> [" << name_ << "] - [errno: " << err
               << "; errmsg: " << strerror_tl(err) << "]";
}

template <typename D>
void ConnectionBase<D>::HandleClose()
{
  loop_->AssertInThread();
  // Connected ==> read event handling
  // Disconnecting ==> call ForceClose()
  assert(state_ == kConnected || state_ == kDisconnecting);
  state_ = kDisconnected;

  LOG_DEBUG_KANON << "The connection [" << name_ << "] is disconnected";
  // ! You can't remove channel in event handling phase.
  // ! Instead, close_callback_ should delay the remove to
  // ! functor calling phase
  channel_->DisableAll();

  // Prevent connection to be removed from TcpServer immediately(since
  // close_callback_) TcpServer::RemoveConnection need to call
  // ConnectionBase<D>::ConnectionDestroyed Therefore, we must guard here
  const auto guard = this->shared_from_this();
  connection_callback_(guard);

  // TcpServer remove connection from its connections_
  if (close_callback_) {
    close_callback_(guard);
  }
}

template <typename D>
void ConnectionBase<D>::ShutdownWrite() noexcept
{
  if (state_ == kConnected) {
    state_ = kDisconnecting;

    // Pass raw pointer is safe.
    loop_->RunInLoop([this]() {
      // If channel is writing state, we need write all message to
      // the buffer in kernel space, in case peer half-close can
      // also receive message
      if (!channel_->IsWriting() || loop_->IsEdgeTriggerMode()) {
        socket_->ShutdownWrite();
      }
    });
  }
}

template <typename D>
void ConnectionBase<D>::ForceClose() noexcept
{
  // If connection is not established or
  // disconnected,
  // ForceClose() do nothing.
  if (state_ == kConnected || state_ == kDisconnecting) {
    state_ = kDisconnecting;

    loop_->RunInLoop([this]() {
      loop_->AssertInThread();
      HandleClose();
    });
  }
}

template <typename D>
void ConnectionBase<D>::Send(void const *data, size_t len)
{
  if (state_ == kConnected) {
    if (loop_->IsLoopInThread()) {
      SendInLoop(data, len);
    } else {
      std::string str(static_cast<char const *>(data), len);

      // Because std::bind can't infer the type of overloaded member function,
      // I modify this function signature.
      // Also, don't use SendInLoopForStr, as following is also OK:
      // void ConnectionBase<D>::SendInLoop(std::string&);
      // (void) (ConnectionBase<D>::* fp) (std::string&) =
      // ConnectionBase<D>::SendInLoop; loop_->QueueToLoop(std::bind(fp, this,
      // std::move(str)));

      loop_->QueueToLoop(std::bind(&ConnectionBase<D>::SendInLoopForStr,
                                   this->shared_from_this(), std::move(str)));
    }
  } else {
    LOG_TRACE_KANON << "Connection [" << name_ << "](fd = " << channel_->GetFd()
                    << ") is down\n"
                    << "state(" << State2String() << "), stop send";
  }
}

template <typename D>
void ConnectionBase<D>::Send(StringView data)
{
  Send(data.data(), data.size());
}

template <typename D>
void ConnectionBase<D>::Send(InputBuffer &buf)
{
  if (state_ == kConnected) {
    if (loop_->IsLoopInThread()) {
      // No need to call swap() even even though output_buffer_ is empty.
      // We just use it in this loop, the remaining contents will cached
      // in output_buffer_
      SendInLoopForBuf(buf);
    } else {
      // The reason for use std::bind instead of lambda is
      // lambda expression doesn't support move capture in
      // C++11, we can't move resource in the capture list
      loop_->QueueToLoop(std::bind(&ConnectionBase<D>::SendInLoopForBuf,
                                   this->shared_from_this(), std::move(buf)));
    }
  } else {
    LOG_TRACE_KANON << "Connection [" << name_ << "](fd = " << channel_->GetFd()
                    << ") is down\n"
                    << "state(" << State2String() << "), stop send";
  }
}

template <typename D>
void ConnectionBase<D>::Send(OutputBuffer &buffer)
{
  if (!IsConnected()) {
    LOG_TRACE_KANON << "Connection [" << name_ << "](fd = " << channel_->GetFd()
                    << ") is down\n"
                    << "state(" << State2String() << "), stop send";
    return;
  }

  if (loop_->IsLoopInThread()) {
    SendInLoopForChunkList(buffer);
  } else {
    loop_->QueueToLoop(std::bind(&ConnectionBase::SendInLoopForChunkList,
                                 this->shared_from_this(), std::move(buffer)));
  }
}

template <typename D>
void ConnectionBase<D>::SendInLoopForStr(std::string &data)
{
  const auto content = std::move(data);
  SendInLoop(content.data(), content.size());
}

template <typename D>
void ConnectionBase<D>::SendInLoop(StringView data)
{
  SendInLoop(data.data(), data.size());
}

template <typename D>
void ConnectionBase<D>::DisbaleRead()
{
  if (state_ == kConnected && channel_->IsReading()) {
    channel_->DisableReading();
  }
}

template <typename D>
void ConnectionBase<D>::EnableRead()
{
  if (state_ == kConnected && !channel_->IsReading()) {
    channel_->EnableReading();
  }
}

template <typename D>
char const *ConnectionBase<D>::State2String() const noexcept
{
  switch (state_) {
    case kConnecting:
      return "Connecting";
    case kConnected:
      return "Connected";
    case kDisconnecting:
      return "Disconnecting";
    case kDisconnected:
      return "Disconnected";
    default:
      return "Invalid State";
  }
}

template <typename D>
void ConnectionBase<D>::SetChannel(std::unique_ptr<Channel> ch)
{
  ch->SetEvents(channel_->GetEvents());
  channel_ = std::move(ch);
  channel_->SetReadCallback(std::bind(&ConnectionBase::HandleRead, this, _1));
  channel_->SetWriteCallback(std::bind(&ConnectionBase::HandleWrite, this));
  channel_->SetErrorCallback(std::bind(&ConnectionBase::HandleError, this));
  channel_->SetCloseCallback(std::bind(&ConnectionBase::HandleClose, this));
}
#ifdef KANON_ON_WIN
#include "kanon/win/net/connection/connection_base.inl"
#elif defined(KANON_ON_UNIX)
#include "kanon/linux/net/connection/connection_base.inl"
#endif

#include "tcp_connection.h"
#include "unix_connection.h"

namespace kanon {

template class ConnectionBase<UnixConnection>;
template class ConnectionBase<TcpConnection>;

} // namespace kanon

//! Although Send() and SendInLoop() check the state of connection,
//! peer can also close connection when server is busy.
//!
//! When peer close connection, if you continue write the closed fd which
//! will receive SIGPIPE but this signal default handler is terminal the process
//! so we should ignore it
//!
//! The dtor is trivial, So define static variable is OK
//! \see test/SIGPIPE_test

#ifdef KANON_ON_UNIX
struct IgnoreSignalPipe {
  IgnoreSignalPipe() { ::signal(SIGPIPE, SIG_IGN); }
};

static IgnoreSignalPipe dummy_ignore_singal_pipe{};
#endif
