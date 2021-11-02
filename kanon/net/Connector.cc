#include "kanon/net/Connector.h"

#include "kanon/net/sock_api.h"
#include "kanon/net/EventLoop.h"
#include "kanon/net/Channel.h"

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

Connector::~Connector() KANON_NOEXCEPT {
  LOG_DEBUG << "Connector destructed";
  // user should call stop explicitly when reconnecting...
  assert(!channel_);
} 

void
Connector::start() KANON_NOEXCEPT {
  loop_->runInLoop([this]() {
    loop_->assertInThread();

    if (connect_) {
      connect();
    } else {
      setState(State::kDisconnected);
    }
  });
}

void
Connector::stop() KANON_NOEXCEPT {
  loop_->runInLoop([this]() {
    loop_->assertInThread();
  
    // Only be called when connecting,
    // interrupt reconnecting to peer 
    if (state_ == State::kConnecting) {
      connect_ = false;
      setState(State::kDisconnected);

      int sockfd = removeAndResetChannel();
      sock::close(sockfd); 
    }
  });
}

void
Connector::restart() KANON_NOEXCEPT {
  connect_ = false;
  setState(State::kDisconnected);
  retryInterval_ = INIT_RETRY_INTERVAL;

  start();
}

void
Connector::connect() KANON_NOEXCEPT {
  int sockfd = sock::createNonBlockAndCloExecSocket(!servAddr_.isIpv4());
  auto ret = sock::connect(sockfd, sock::to_sockaddr(servAddr_.toIpv6()));

  auto saved_errno = (ret == 0) ? 0 : errno;

  switch(saved_errno) {
    case 0:
    case EINTR:
    case EINPROGRESS: // connecting...(only for nonblocking)
    case EISCONN: // connected(continues harmlessly)
      completeConnect(sockfd);
      break;

    case EAGAIN: // ephemeral port have run out
    case EADDRINUSE:
    case EADDRNOTAVAIL: // no port can be used(or enlarge port range?)
    case ENETUNREACH:
    case ECONNREFUSED: // accept RST packet
      retry(sockfd);
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
      LOG_SYSERROR << "connect error in Connector::connect()";
      sock::close(sockfd);
      break;
    default:
      LOG_SYSERROR << "unknown error in Connector::connect()";
      sock::close(sockfd);
  }

}

void
Connector::completeConnect(int sockfd) KANON_NOEXCEPT {
  if (state_ == State::kDisconnected) {
    setState(State::kConnecting);

    assert(!channel_);
    channel_ = kanon::make_unique<Channel>(loop_, sockfd);

    channel_->setWriteCallback([this]() {
      if (state_ == State::kConnecting) {
        int sockfd = removeAndResetChannel();
        int err = sock::getsocketError(sockfd);

        if (err) {
          // Fatal errors have handled in connect()
          LOG_WARN << "SO_ERROR = " << err 
            << " " << strerror_tl(err);
          // does not complete, retry
          retry(sockfd);
        } else if (sock::isSelfConnect(sockfd)) {
          LOG_WARN << "self connect";
          retry(sockfd);
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

    channel_->setErrorCallback([this]() {
      if (state_ == State::kConnecting) {
        int sockfd = removeAndResetChannel();
        int err = sock::getsocketError(sockfd);
        if (err) {
          LOG_TRACE << "SO_ERROR = " << err << " " 
            << strerror_tl(err);
        }

        setState(State::kDisconnected);
        retry(sockfd);
      }   
    });

    channel_->enableWriting();

  }
}

void
Connector::retry(int sockfd) KANON_NOEXCEPT {
  sock::close(sockfd);

  if (connect_) {
    double delaySec = (MIN(retryInterval_, MAX_RETRY_INTERVAL)) / 1000.0;

    LOG_INFO << "Client will reconnect to " << servAddr_.toIpPort()
      << " after " << delaySec << " seconds";

    loop_->runAfter([this]() {
      start();
    }, delaySec);

    retryInterval_ *= 2;  
  }
}

int
Connector::removeAndResetChannel() KANON_NOEXCEPT {
  loop_->assertInThread();

  int sockfd = channel_->fd();
  channel_->disableAll();
  channel_->remove();

  // @warning 
  // in event handle phase now
  // So, you can't call it immediately
  loop_->queueToLoop([this]() {
    channel_.reset();
  });

  return sockfd;
}
