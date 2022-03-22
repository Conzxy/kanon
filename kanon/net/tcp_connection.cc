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
  connection_callback_(shared_from_this());
}

void TcpConnection::ConnectionDestroyed() {
  loop_->AssertInThread();

  // This may be called by TcpServer dtor or close_callback_(see TcpServer)
  // if close_callback_ has be called, just remove channel;  
  if (state_ == kConnected) {
    // channel_->DisableAll();
    state_ = kDisconnected;

    // Because ConnectionDestroyed maybe async call
    // Don't pass raw pointer
    connection_callback_(shared_from_this());  
  }  
 
  channel_->Remove();
}

void TcpConnection::HandleRead(TimeStamp recv_time) {
  /**
    * 1. Call ReadFd() to get data
    * 2. Check return value
    * 2.1. If 0, indicates peer close connection, call close_callback_
    * 2.2. >0, call message_callback_
    * 2.3. <0, error occurred, call HandleError()
    */
  loop_->AssertInThread();
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
    LOG_DEBUG << "Peer close connection";
    HandleClose();
  } else {
    errno = saved_errno;

    if (errno != EAGAIN) {
      LOG_SYSERROR << "Read event handle error";
      HandleError();
    }
  }
}

void TcpConnection::HandleWrite() {
  /**
    * Logical is simple, just write the contents in ouput_buffer_
    * Normally, this be called when 
    * * Short write 
    * * Active write(e.g. output_buffer_ provided by user)
    * ! No need to handle error, since read callback will handle(read() return 0)
    */
  loop_->AssertInThread();
  assert(channel_->IsWriting());  

  // FIXME example 
  // Here shouldn't use socket_->GetFd(),
  // because socket maybe has destroyed when peer close early
  auto n = sock::Write(channel_->GetFd(),
            output_buffer_.GetReadBegin(),
            output_buffer_.GetReadableSize());

  LOG_TRACE << "sock::write " << n << " bytes"; 

  if (n > 0) {
    output_buffer_.AdvanceRead(n);

    LOG_TRACE << "Ouput Buffer remaining = " << output_buffer_.GetReadableSize();
    if (output_buffer_.GetReadableSize() == 0) {
      channel_->DisableWriting();
      if (write_complete_callback_) {
        // We delay the callback to phase 3
        // to increase the response rate
        loop_->QueueToLoop([this]() {
          write_complete_callback_(shared_from_this());
        });
      }
    }
  } else {
    LOG_SYSERROR << "write event handle error";
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
  loop_->RunInLoop([=]() {
    socket_->ShutdownWrite();
  });
}

void TcpConnection::ForceClose() noexcept {
  loop_->RunInLoop([=]() {
      loop_->AssertInThread();
      
      if (state_ & (kConnected | kDisconnecting)) {
        state_ = kDisconnecting;
        HandleClose(); 
      }
  });
}

void TcpConnection::Send(void const* data, size_t len) {
  Send(StringView{ 
      static_cast<char const*>(data), 
      static_cast<StringView::size_type>(len) });
}

void TcpConnection::Send(StringView data) {
  if (state_ == kConnected) {
    if (loop_->IsLoopInThread()) {
      SendInLoop(data);
    } else {
      loop_->QueueToLoop([=]() {
        this->SendInLoop(std::string(data.data(), data.size()));
      });
    }
  }
}

void TcpConnection::Send(Buffer& buf) {
  if (state_ == kConnected) {
    if (loop_->IsLoopInThread()) {
      // No need to call swap() even even though output_buffer_ is empty.
      // We just use it in this loop, the remaining contents will cached 
      // in output_buffer_
      SendInLoop(buf.GetReadBegin(), buf.GetReadableSize());
      buf.AdvanceRead(buf.GetReadableSize());
    } else {
      if (!output_buffer_.HasReadable()) {
        buf.swap(output_buffer_);
        // Also, mutiple calls EnableWriting() is harmless
        // But the syscall is not cheap.
        loop_->QueueToLoop([this]() {
          if (!channel_->IsWriting()) {
            channel_->EnableWriting();
          }
        });
      }
      else {
        const auto content = buf.RetrieveAllAsString();
        // In C++11, lambda don't support move capture

        // loop_->QueueToLoop([this, content]() {
        //   SendInLoop(content);
        // });

        loop_->QueueToLoop(std::bind(
          &TcpConnection::SendInLoopForStr, this, std::move(content)));
      }
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
  LOG_TRACE << "The length of data parameter is " << len;

  ssize_t n = 0;
  size_t remaining = len;

  // if is not writing state, indicates output_buffer_ maybe is empty,
  // but also output_buffer_ is filled by user throught GetOutputBuffer().
  // When it is not writing state and output_buffer_ is empty,
  // we can write directly first
  if (!channel_->IsWriting() && output_buffer_.GetReadableSize() == 0) {
    n = sock::Write(channel_->GetFd(), data, len);

    LOG_TRACE << "sock::write " << n << " bytes";

    if (n >= 0) {
      if (static_cast<size_t>(n) != len) {
        remaining -= n;  
      } else {
        if (write_complete_callback_) {
          write_complete_callback_(shared_from_this());
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
      loop_->QueueToLoop([this, readable_len, remaining]() {
        high_water_mark_callback_(shared_from_this(), readable_len + remaining);
      });
    }

    LOG_TRACE << "Remaining content length = " << remaining;    
    output_buffer_.Append(static_cast<char const*>(data)+n, remaining);
    if (!channel_->IsWriting()) {
      channel_->EnableWriting();
    }
  }
}

void TcpConnection::SetNoDelay(bool flag) noexcept
{ socket_->SetNoDelay(flag); }

void TcpConnection::SetKeepAlive(bool flag) noexcept
{ socket_->SetKeepAlive(flag); }

// When peer close connection, if you continue write the closed fd which
// will receive SIGPIPE but this signal default handler is terminal the process
// so we should ignore it
//
// The dtor is trivial, So define static variable is OK
// @see test/SIGPIPE_test
struct IgnoreSignalPipe {
  IgnoreSignalPipe() {
    ::signal(SIGPIPE, SIG_IGN);
  }
};

static IgnoreSignalPipe ignore_singal_pipe{};