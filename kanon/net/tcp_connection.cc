#include "kanon/net/tcp_connection.h"

#include <errno.h>
#include <signal.h> // SIGPIPE, signal()
#include <atomic>

#include "kanon/net/socket.h"
#include "kanon/net/channel.h"
#include "kanon/net/event_loop.h"


using namespace kanon;

static constexpr int kDefaultHighWatermark = 64 * 1024;

char const* const
TcpConnection::state_str_[STATE_NUM] = {
  "connecting",
  "connected",
  "disconnecting",
  "disconnected"
};

TcpConnection::TcpConnection(EventLoop*  loop,
               std::string const& name,
               int sockfd,
               InetAddr const& local_addr,
               InetAddr const& peer_addr)
  : loop_{ loop }
  , name_{ name }
  , socket_{ kanon::make_unique<Socket>(sockfd) }
  , channel_{ kanon::make_unique<Channel>(loop, sockfd) }
  , local_addr_{ local_addr }
  , peer_addr_{ peer_addr }
  , high_water_mark_{ kDefaultHighWatermark }
  , state_{ kConnecting }
{
  channel_->SetReadCallback(std::bind(
    &TcpConnection::HandleRead, this, _1));

  channel_->SetWriteCallback(std::bind(
    &TcpConnection::HandleWrite, this));

  channel_->SetErrorCallback(std::bind(
    &TcpConnection::HandleError, this));
  
  channel_->SetCloseCallback(std::bind(
    &TcpConnection::HandleClose, this));

  LOG_TRACE << "TcpConnection::ctor [" << name_ << "] created";
  
}

TcpConnection::~TcpConnection() noexcept {
  assert(state_ == kDisconnected);
  LOG_TRACE << "TcpConnection::dtor [" << name_ << "] destroyed";
}

void TcpConnection::ConnectionEstablished() {
  loop_->AssertInThread();

  assert(state_ == kConnecting);
  state_ = kConnected;

  // Start observing read event on this socket  
  // and call the OnConnection callback which
  // is set by user or default.
  channel_->EnableReading();  

  LOG_TRACE << "Connection [" << name_ << "] is established";
  connection_callback_(shared_from_this());
}

void TcpConnection::ConnectionDestroyed() {
  // ! Must be called in phase 3(QueueToLoop())
  loop_->AssertInThread();

  // This may be called by TcpServer dtor or close_callback_(see TcpServer)
  // if close_callback_ has be called, just remove channel;  
  if (state_ == kConnected) {
    channel_->DisableAll();
    state_ = kDisconnected;

    LOG_TRACE << "Connection [" << name_ << "] has destroyed";
    // Because ConnectionDestroyed maybe async call
    // Don't pass raw pointer
    connection_callback_(shared_from_this());
  }  

  assert(state_ == kDisconnected);

  // To Epoller, this is trivial operation
  // To Poller, this will remove connection in its data structure
  channel_->Remove();
}

void TcpConnection::HandleRead(TimeStamp recv_time) 
{
  loop_->AssertInThread();
  if (loop_->IsEdgeTriggerMode()) {
    HandleEtRead(recv_time);
  }
  else {
    HandleLtRead(recv_time);
  }
}

void TcpConnection::HandleLtRead(TimeStamp recv_time) {
  /**
    * 1. Call ReadFd() to get data
    * 2. Check return value
    * 2.1. If 0, indicates peer close connection, call close_callback_
    * 2.2. >0, call message_callback_
    * 2.3. <0, error occurred, call HandleError()
    */
  int saved_errno;
  auto n = input_buffer_.ReadFd(channel_->GetFd(), saved_errno);
  
  LOG_DEBUG << "Read " << n << " bytes from fd = " << channel_->GetFd(); 

  if (n > 0) {
    if (message_callback_) {
      message_callback_(shared_from_this(), input_buffer_, recv_time);
    } else {
      input_buffer_.AdvanceAll();
      LOG_WARN << "If user want to process message from peer, should set "
                  "proper message_callback_";
    }
  } else if (n == 0) {
    // Peer close the connection
    // Note: Don't distinguish the shutdown(WR) and close()
    LOG_TRACE << "Peer close connection";
    HandleClose();
  } else {
    errno = saved_errno;

    if (errno != EAGAIN) {
      LOG_SYSERROR << "Read event handle error";
      HandleError();
    }
  }
}

void TcpConnection::HandleEtRead(TimeStamp recv_time)
{
  // The event occurred only in the:
  // 1. new message coming
  // 2. buffer zero to low-watermark

  // 1. Call input_buffer_.ReadFd() get some message
  // 2.1. If the saved_errno is less than 0, and isn't EAGAIN(OR EWOULDBLOCK),
  //    continue read until the errno = EAGAIN 
  // 2.2. If the saved_errno is not EAGAIN, it indicates there is a error occurred
  // 3. The message length is greater than 0, call the message_callback_ which is user-defined
  //    If not a valid callback(i.e. empty callback) is also ok, just discard the contents in
  //    input_buffer_
  // 4. The message length is equal to 0, it dicates that peer close this connection
  //    then call HandleClose() since this is passive close
  int saved_errno = 0;

  for (; ; ) {
    auto readn = input_buffer_.ReadFd(channel_->GetFd(), saved_errno);

    if (saved_errno < 0) {
      if (saved_errno != EAGAIN) {
        LOG_SYSERROR << "Read event handle error";
        HandleError();
      }
      else {
        break;
      }
    }

    if (readn == 0) {
      LOG_DEBUG << "Peer close connection";
      HandleClose();
      break;
    }

    // FIXME
    // message_callback_ should be called here?
    if (message_callback_) {
      message_callback_(shared_from_this(), input_buffer_, recv_time);
    }
    else {
      input_buffer_.AdvanceAll();
      LOG_WARN << "If user want to process message from peer, should set "
                  "proper message_callback_";
    }
  }
  
}

void TcpConnection::HandleWrite()
{
  loop_->AssertInThread();

  // HandleClose() is called OR server/client is destoryed
  // 1. HandleClose() call DisableAll() 
  // 2. ConnectionDestoryed() is called when connection is active
  //    (e.g. TcpServer desctroyed)
  if (!channel_->IsWriting()) {
    assert(state_ == kConnected);
    LOG_TRACE << "This connection is down";
    return ;
  }

  if (loop_->IsEdgeTriggerMode()) {
    HandleEtWrite();
  }
  else {
    HandleLtWrite();
  }
}

void TcpConnection::HandleLtWrite() {
  /**
    * Logical is simple, just write the contents in ouput_buffer_
    * Normally, this be called when 
    * 1. Short write 
    * 2. Actively write(e.g. output_buffer_ provided by user)
    */

  // FIXME example 
  // Here shouldn't use socket_->GetFd(),
  // because socket maybe has destroyed when peer close early

  // The first write don't get EAGAIN
  // If the first write don't write entire message, the second will return EAGAIN
  // Otherwise, there is no need to call the second call of write().
  // So, just call write once is enough
  auto n = sock::Write(channel_->GetFd(),
            output_buffer_.GetReadBegin(),
            output_buffer_.GetReadableSize());

  LOG_TRACE << "Write " << n << " bytes"; 

  if (n > 0) {
    output_buffer_.AdvanceRead(n);

    LOG_TRACE << "Ouput Buffer remaining = " << output_buffer_.GetReadableSize();
    if (output_buffer_.GetReadableSize() == 0) {
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
        //   write_complete_callback_, shared_from_this()));
        loop_->QueueToLoop(std::bind(
          &TcpConnection::CallWriteCompleteCallback, this));
      }
      else {
        channel_->DisableWriting();
      }

      // Disconnecting is set in ShutdownWrite(). Because there are some message
      // need write(channel_->IsWriting()) to kernel space, delay the shutdown()
      // to here
      if (state_ == kDisconnecting) {
        ShutdownWrite();
      }
    }
  } else {
    LOG_SYSERROR << "Write event handle error";
    HandleError();
  }

}

void TcpConnection::CallWriteCompleteCallback()
{
  if (write_complete_callback_(shared_from_this())) {
    channel_->DisableWriting();
  }
}

void TcpConnection::HandleEtWrite()
{
  // The event occurred only in the:
  // 1. buffer high-watermark to low-watermark(writable)

  auto writen = sock::Write(
    channel_->GetFd(),
    output_buffer_.GetReadBegin(),
    output_buffer_.GetReadableSize());
  
  LOG_TRACE << "Write " << writen << " bytes"; 
  if (writen > 0) {
    output_buffer_.AdvanceRead(writen);

    if (output_buffer_.HasReadable()) {
      LOG_TRACE << "Ouput Buffer remaining = " << output_buffer_.GetReadableSize();
      // To write entire message, we need resigter write event again
      channel_->EnableWriting();
    }
    else {
      if (write_complete_callback_) {
        // No need to disable writing
        loop_->QueueToLoop(std::bind(
          write_complete_callback_, shared_from_this()));
      }

      if (state_ == kDisconnecting) {
        socket_->ShutdownWrite();
      }
    }
  }
  else {
    // writen == 0 is also error, since output_buffer_ must not be empty when write event occurred
    LOG_SYSERROR << "Write event handle error";
    HandleError();
  }
}

void TcpConnection::HandleError() {
  loop_->AssertInThread();
  int err = sock::GetSocketError(channel_->GetFd());
  LOG_SYSERROR << "TcpConnection [" << name_ << "] - [errno: " 
    << err << "; errmsg: " << strerror_tl(err) << "]";
}

void TcpConnection::HandleClose() {
  loop_->AssertInThread();
  // Connected ==> read event handling
  // Disconnecting ==> call ForceClose()
  assert(state_ & (kConnected | kDisconnecting));
  
  state_ = kDisconnected;

  // ! You can't remove channel in event handling phase.
  // ! Instead, close_callback_ should delay the remove to
  // ! functor calling phase
  channel_->DisableAll();
  
  // Prevent connection to be removed from TcpServer immediately(since close_callback_)
  // TcpServer::RemoveConnection need to call TcpConnection::ConnectionDestroyed
  // Therefore, we must guard here
  const auto guard = shared_from_this();  
  connection_callback_(guard);
  
  // TcpServer remove connection from its connections_
  if (close_callback_) {
    close_callback_(guard);
  }
}

void TcpConnection::ShutdownWrite() noexcept {
  state_ = kDisconnecting;

  loop_->RunInLoop([this]() {
    // If channel is writing state, we need write all message to
    // the buffer in kernel space, in case peer half-close can 
    // also receive message
    if (!channel_->IsWriting() || loop_->IsEdgeTriggerMode()) {
      socket_->ShutdownWrite();
    }
  });
}

void TcpConnection::ForceClose() noexcept {
  loop_->RunInLoop([=]() {
      loop_->AssertInThread();

      if (state_ & (kConnected)) {
        state_ = kDisconnecting;

        // socket_->ShutdownTwoDirection();
        HandleClose(); 
      }
  });
}

void TcpConnection::Send(void const* data, size_t len) {
  if (state_ == kConnected) {
    if (loop_->IsLoopInThread()) {
      SendInLoop(data, len);
    }
    else {
      std::string str(static_cast<char const*>(data), len);

      // Because std::bind can't infer the type of overloaded member function,
      // I modify this function signature.
      // Also, don't use SendInLoopForStr, as following is also OK:
      // void TcpConnection::SendInLoop(std::string&);
      // (void) (TcpConnection::* fp) (std::string&) = TcpConnection::SendInLoop;
      // loop_->QueueToLoop(std::bind(fp, this, std::move(str)));

      loop_->QueueToLoop(std::bind(
        &TcpConnection::SendInLoopForStr, this, std::move(str)));
    }
  }
}

void TcpConnection::Send(StringView data) {
  Send(data.data(), data.size());
}

void TcpConnection::Send(Buffer& buf) {
  if (state_ == kConnected) {
    if (loop_->IsLoopInThread()) {
      // No need to call swap() even even though output_buffer_ is empty.
      // We just use it in this loop, the remaining contents will cached 
      // in output_buffer_
      SendInLoopForBuf(buf);
    } else {
      Buffer buffer;
      buffer.swap(buf);

      // The reason for use std::bind instead of lambda is
      // lambda expression doesn't support move capture in
      // C++11, we can't move resource in the capture list 
      loop_->QueueToLoop(std::bind(
        &TcpConnection::SendInLoopForBuf, this, std::move(buffer)
      ));
    }
  }
}

void TcpConnection::SendInLoopForStr(std::string& data) {
  const auto content = std::move(data);
  SendInLoop(content.data(), content.size());
}

void TcpConnection::SendInLoop(StringView data) {
  SendInLoop(data.data(), data.size());
}

void TcpConnection::SendInLoop(void const* data, size_t len) {
  ssize_t n = 0;
  size_t remaining = len;

  // Although Send() has checked state_ is kConnected
  // But connection also can be closed in the phase 2 
  // when this is called in phase 3
  if (state_ & ~(kConnected)) {
    LOG_WARN << "This connection is not connected, don't send any message";
    return ;
  }

  // if is not writing state, indicates output_buffer_ maybe is empty,
  // but also output_buffer_ is filled by user throught GetOutputBuffer().
  // When it is not writing state and output_buffer_ is empty,
  // we can write directly first
  if (!channel_->IsWriting() && output_buffer_.GetReadableSize() == 0) {
    n = sock::Write(channel_->GetFd(), data, len);


    if (n >= 0) {
      LOG_TRACE << "write " << n << " bytes";
      if (static_cast<size_t>(n) != len) {
        remaining -= n;  
      } else {
        if (write_complete_callback_) {
          loop_->QueueToLoop(std::bind(
            write_complete_callback_, shared_from_this()));
        }
        
        return ;
      }
    } else {
      if (errno != EAGAIN) // EWOULDBLOCK
        LOG_SYSERROR << "write unexpected error occurred";
      return ;
    }
  }

  if (remaining > 0) {
    // short write happened
    // Store remaing message to output_buffer_
    // then write callback will handle
    auto readable_len = output_buffer_.GetReadableSize();

    if (readable_len + remaining >= high_water_mark_ &&
        readable_len < high_water_mark_ &&
        high_water_mark_callback_) {
      loop_->QueueToLoop(std::bind(
        high_water_mark_callback_, shared_from_this(), readable_len + remaining));
      // loop_->QueueToLoop([this, readable_len, remaining]() {
      //   high_water_mark_callback_(shared_from_this(), readable_len + remaining);
      // });
    }

    LOG_TRACE << "Remaining content length = " << remaining;    
    output_buffer_.Append(static_cast<char const*>(data)+n, remaining);
    if (!channel_->IsWriting()) {
      channel_->EnableWriting();
    }
  }
}

void TcpConnection::SendInLoopForBuf(Buffer& buffer) {
  if (state_ & ~(kConnected)) {
    LOG_WARN << "This connection is not connected, don't send any message";
    return ;
  }

  if (!channel_->IsWriting() && !output_buffer_.HasReadable()) {
    output_buffer_.swap(buffer);

    auto n = sock::Write(
      channel_->GetFd(), 
      output_buffer_.ToStringView().data(),
      output_buffer_.GetReadableSize());

    if (n >= 0) {
      LOG_TRACE << "Write length = " << n;
      output_buffer_.AdvanceRead(n);
      if (static_cast<size_t>(0) != output_buffer_.GetReadableSize()) {
        LOG_TRACE << "Remaining length = " << output_buffer_.GetReadableSize();

        if (output_buffer_.GetReadableSize() >= high_water_mark_ &&
            high_water_mark_callback_) {
          loop_->QueueToLoop(std::bind(
            high_water_mark_callback_, shared_from_this(), output_buffer_.GetReadableSize()));
        }
        channel_->EnableWriting();
      }
      else {
        if (write_complete_callback_) {
          loop_->QueueToLoop(std::bind(
            write_complete_callback_, shared_from_this()
          ));
        }
      }

    }
    else {
      if (errno != EAGAIN) {
        LOG_SYSERROR << "write unexpected error occurred";
      }

      return ;
    }
  }
  else {
    SendInLoop(buffer.ToStringView());
  }
}

void TcpConnection::SetNoDelay(bool flag) noexcept
{ socket_->SetNoDelay(flag); }

void TcpConnection::SetKeepAlive(bool flag) noexcept
{ socket_->SetKeepAlive(flag); }

// Although Send() and SendInLoop() check the state of connection,
// peer can also close connection when server is busy.
// 
// When peer close connection, if you continue write the closed fd which
// will receive SIGPIPE but this signal default handler is terminal the process
// so we should ignore it
//
// The dtor is trivial, So define static variable is OK
// \see test/SIGPIPE_test
struct IgnoreSignalPipe {
  IgnoreSignalPipe() {
    ::signal(SIGPIPE, SIG_IGN);
  }
};

static IgnoreSignalPipe dummy_ignore_singal_pipe{};
