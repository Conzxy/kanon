#include "kanon/net/connector.h"

#include "kanon/net/sock_api.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/channel.h"

using namespace kanon;

#define INIT_RETRY_INTERVAL 500
#define MAX_RETRY_INTERVAL 30000

Connector::Connector(
  EventLoop* loop,
  InetAddr const& serv_addr) 
  : loop_{ loop }
  , serv_addr_{ serv_addr }
  , retry_interval_{ INIT_RETRY_INTERVAL }
  , state_{ State::kDisconnected }
  , connect_{ true }
{
  LOG_DEBUG << "Connector is constructed";
}

Connector::~Connector() noexcept {
  LOG_DEBUG << "Connector is destructed";
  // user should call stop explicitly when reconnecting...
  assert(!channel_);
} 

void Connector::StartRun() noexcept {
  loop_->RunInLoop([this]() {
    loop_->AssertInThread();

    if (connect_) {
      Connect();
    } else {
      SetState(State::kDisconnected);
    }
  });
}

void Connector::Stop() noexcept {
  // This is unsafe
  // Scenario:
  // one thread(usually, main thread) desctroy
  // TcpClient and call Stop() in its dtor, but
  // Connector has destroyed together, then the
  // loop_ become invalid address, access will 
  // trigger AssertInthread() to abort entire program.
  // (this is called in the phase3 as a callback)
  // Solution:
  // Make connector managed by std::shared_ptr
  connect_ = false;

  loop_->RunInLoop(std::bind(&Connector::StopInLoop, shared_from_this()));
}

void Connector::StopInLoop() noexcept
{ 
  loop_->AssertInThread();

  LOG_TRACE << "Connector is stopping";
  // Only be called when connecting,
  // interrupt reconnecting to peer 
  SetState(State::kDisconnected);
  if (timer_) loop_->CancelTimer(*timer_);

  if (state_ == State::kConnecting) {
    int sockfd = RemoveAndResetChannel();
    sock::Close(sockfd); 
  }
}

void Connector::Restrat() noexcept {
  connect_ = true;

  assert(state_ == kConnected);
  SetState(State::kDisconnected);
  retry_interval_ = INIT_RETRY_INTERVAL;
  StartRun();
}

void Connector::Connect() noexcept {
  int sockfd = sock::CreateNonBlockAndCloExecSocket(!serv_addr_.IsIpv4());
  // Poll to check connection if is established
  // If connection is established, we call CompleteConnect() to register write callback,
  // then write callback will call new_connection_callback_.
  // Otherwise, call Retry() to connect again.
  auto ret = sock::Connect(sockfd, serv_addr_.ToSockaddr());

  auto saved_errno = (ret == 0) ? 0 : errno;

  // \see man connect
  switch(saved_errno) {
    case 0:
    case EINTR:
    case EINPROGRESS: // Connecting(Only for nonblocking)
    case EISCONN: // Connected(continues harmlessly)
      CompleteConnect(sockfd);
      break;

    case EAGAIN: // For tcp, there are insufficient entries in the routing cache
    case EADDRINUSE:
    case EADDRNOTAVAIL: // No port can be used(or enlarge port range?)
    case ENETUNREACH:
    case ECONNREFUSED: // Accept RST segments, unreachable address
      Retry(sockfd);
      break;

    default:
      LOG_SYSERROR << "Unexpected error in Connector::Connect()";
      sock::Close(sockfd);
  }

}

void Connector::CompleteConnect(int sockfd) noexcept {
  // If user call Stop(), shouldn't continue complete connect
  if (state_ == State::kDisconnected && connect_) {
    SetState(State::kConnecting);

    assert(!channel_);
    channel_ = kanon::make_unique<Channel>(loop_, sockfd);

    channel_->SetWriteCallback([this]() {
      if (state_ == State::kConnecting) {
        int sockfd = RemoveAndResetChannel();
        int err = sock::GetSocketError(sockfd);

        if (err) {
          // Fatal errors have handled in Connect()
          LOG_WARN << "SO_ERROR = " << err 
                   << " " << strerror_tl(err);
          Retry(sockfd);
        } else if (sock::IsSelfConnect(sockfd)) {
          LOG_WARN << "self connect";
          Retry(sockfd);
        } else {
          if (connect_) {
            // new_connection_callback should be seted by client
            SetState(State::kConnected);
            if (new_connection_callback_) 
              new_connection_callback_(sockfd);

            LOG_TRACE << "Connection is established successfully";
          } else {
            sock::Close(sockfd);
          }
        }
      }
    });

    channel_->SetErrorCallback([this]() {
      if (state_ == State::kConnecting) {
        int sockfd = RemoveAndResetChannel();
        int err = sock::GetSocketError(sockfd);
        if (err) {
          LOG_TRACE << "SO_ERROR = " << err << " " 
            << strerror_tl(err);
        }
        
        Retry(sockfd);
      }   
    });

    channel_->EnableWriting();

  }
}

void Connector::Retry(int sockfd) noexcept {
  loop_->AssertInThread();
  SetState(kDisconnected);
  sock::Close(sockfd);

  if (connect_) {
    double delay_sec = std::min<uint32_t>(retry_interval_, MAX_RETRY_INTERVAL) / 1000.0;

    LOG_INFO << "Client will reconnect to " << serv_addr_.ToIpPort()
             << " after " << delay_sec << " seconds";

    timer_ = loop_->RunAfter([this]() {
      StartRun();
    }, delay_sec);

    retry_interval_ *= 2;  
  }
}

int Connector::RemoveAndResetChannel() noexcept {
  // Ensure this is called in loop
  // This can be called in the phase2 or phase3
  loop_->AssertInThread();

  int sockfd = channel_->GetFd();
  channel_->DisableAll();
  channel_->Remove();

  // \warning 
  // In event handle phase now if called in phase2
  // So, you can't call it immediately
  loop_->QueueToLoop([this]() {
    channel_.reset();
  });

  return sockfd;
}
