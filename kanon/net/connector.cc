#include "kanon/net/connector.h"

#include "kanon/net/sock_api.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/channel.h"

using namespace kanon;

#define INIT_RETRY_INTERVAL 500
#define MAX_RETRY_INTERVAL 30000

Connector::Connector(
  EventLoop* loop,
  InetAddr const& servAddr) 
  : loop_{ loop }
  , servAddr_{ servAddr }
  , connect_{ true }
  , state_{ State::kDisconnected }
  , retryInterval_{ INIT_RETRY_INTERVAL }
{
  LOG_DEBUG << "Connector constructed";
}

Connector::~Connector() noexcept {
  LOG_DEBUG << "Connector destructed";
  // user should call stop explicitly when reconnecting...
  assert(!channel_);
} 

void
Connector::StartRun() noexcept {
  loop_->RunInLoop([this]() {
    loop_->AssertInThread();

    if (connect_) {
      Connect();
    } else {
      setState(State::kDisconnected);
    }
  });
}

void
Connector::Stop() noexcept {
  loop_->RunInLoop([this]() {
    loop_->AssertInThread();
  
    // Only be called when connecting,
    // interrupt reconnecting to peer 
    if (state_ == State::kConnecting) {
      connect_ = false;
      setState(State::kDisconnected);

      int sockfd = RemoveAndResetChannel();
      sock::close(sockfd); 
    }
  });
}

void
Connector::Restrat() noexcept {
  connect_ = false;
  setState(State::kDisconnected);
  retryInterval_ = INIT_RETRY_INTERVAL;

  StartRun();
}

void
Connector::Connect() noexcept {
  int sockfd = sock::createNonBlockAndCloExecSocket(!servAddr_.IsIpv4());
  // Poll to check connection if is established
  // If connection is established, we call CompleteConnect() to register write callback,
  // then write callback will call new_connection_callback_.
  // Otherwise, call Retry() to connect again.
  auto ret = sock::Connect(sockfd, sock::to_sockaddr(servAddr_.ToIpv6()));

  auto saved_errno = (ret == 0) ? 0 : errno;

  switch(saved_errno) {
    case 0:
    case EINTR:
    case EINPROGRESS: // connecting...(only for nonblocking)
    case EISCONN: // connected(continues harmlessly)
      CompleteConnect(sockfd);
      break;

    case EAGAIN: // ephemeral port have run out
    case EADDRINUSE:
    case EADDRNOTAVAIL: // no port can be used(or enlarge port range?)
    case ENETUNREACH:
    case ECONNREFUSED: // accept RST packet
      Retry(sockfd);
      break;

    // tried to connect to a broadcast address but not set flag
    // or local firewall rule
    case EACCES: case EPERM:
    case EALREADY:
    case EAFNOSUPPORT: // address family not correct
    case EBADF: // fd is invalid
    case EFAULT:
    case ENOTSOCK: // fd is not a socket
    case EPROTOTYPE:
    case ETIMEDOUT:
      LOG_SYSERROR << "connect error in Connector::Connect()";
      sock::close(sockfd);
      break;
    default:
      LOG_SYSERROR << "unknown error in Connector::Connect()";
      sock::close(sockfd);
  }

}

void
Connector::CompleteConnect(int sockfd) noexcept {
  if (state_ == State::kDisconnected) {
    setState(State::kConnecting);

    assert(!channel_);
    channel_ = kanon::make_unique<Channel>(loop_, sockfd);

    channel_->SetWriteCallback([this]() {
      if (state_ == State::kConnecting) {
        int sockfd = RemoveAndResetChannel();
        int err = sock::getsocketError(sockfd);

        if (err) {
          // Fatal errors have handled in Connect()
          LOG_WARN << "SO_ERROR = " << err 
            << " " << strerror_tl(err);
          // does not complete, retry
          Retry(sockfd);
        } else if (sock::isSelfConnect(sockfd)) {
          LOG_WARN << "self connect";
          Retry(sockfd);
        } else {
          if (connect_) {
            // new_connection_callback should be seted by client
            setState(State::kConnected);
            if (new_connection_callback_) 
              new_connection_callback_(sockfd);
          } else {
            sock::close(sockfd);
          }
        }
      }
    });

    channel_->SetErrorCallback([this]() {
      if (state_ == State::kConnecting) {
        int sockfd = RemoveAndResetChannel();
        int err = sock::getsocketError(sockfd);
        if (err) {
          LOG_TRACE << "SO_ERROR = " << err << " " 
            << strerror_tl(err);
        }

        setState(State::kDisconnected);
        Retry(sockfd);
      }   
    });

    channel_->EnableWriting();

  }
}

void
Connector::Retry(int sockfd) noexcept {
  sock::close(sockfd);

  if (connect_) {
    double delaySec = std::min<uint32_t>(retryInterval_, MAX_RETRY_INTERVAL) / 1000.0;

    LOG_INFO << "Client will reconnect to " << servAddr_.ToIpPort()
      << " after " << delaySec << " seconds";

    loop_->RunAfter([this]() {
      StartRun();
    }, delaySec);

    retryInterval_ *= 2;  
  }
}

int
Connector::RemoveAndResetChannel() noexcept {
  loop_->AssertInThread();

  int sockfd = channel_->GetFd();
  channel_->DisableAll();
  channel_->Remove();

  // @warning 
  // in event handle phase now
  // So, you can't call it immediately
  loop_->QueueToLoop([this]() {
    channel_.reset();
  });

  return sockfd;
}
