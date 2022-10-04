#include "kanon/net/connector.h"

#include "kanon/net/channel.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/sock_api.h"
#include "kanon/util/macro.h"

using namespace kanon;

/* unit: milliseconds */
#define INIT_RETRY_INTERVAL 500
#define MAX_RETRY_INTERVAL 30000

Connector::Connector(EventLoop *loop, InetAddr const &serv_addr)
  : loop_{loop}
  , serv_addr_{serv_addr}
  , retry_interval_{INIT_RETRY_INTERVAL}
  , state_{State::kDisconnected}
  , connect_{true}
{
  LOG_DEBUG_KANON << "Connector is constructed";
}

Connector::~Connector() noexcept
{
  LOG_DEBUG_KANON << "Connector is destructed";
  // user should call stop explicitly when reconnecting...
  assert(!channel_);
}

void Connector::StartRun()
{
  loop_->RunInLoop([this]() {
    loop_->AssertInThread();

    assert(state_ == kDisconnected);
    if (connect_) {
      Connect();
    }
  });
}

void Connector::Stop()
{
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
  loop_->RunInLoop(std::bind(&Connector::StopInLoop, shared_from_this()));
}

void Connector::StopInLoop()
{
  loop_->AssertInThread();
  LOG_TRACE_KANON << "Connector is stopping";

  connect_ = false;
  if (timer_)
    loop_->CancelTimer(*timer_);

  if (state_ == State::kConnecting) {
    // Only be called when connecting,
    // interrupt reconnecting to peer
    int sockfd = channel_->GetFd();
    ResetChannel();
    sock::Close(sockfd);
    SetState(State::kDisconnected);
  }
}

void Connector::Restrat()
{
  loop_->RunInLoop([this]() {
    connect_ = true;
    
    /*
     * Can't distinguish passive close and active close
     * e.g. Connector is destroyed and restart shouldn't be thinked to close
     *      if connection is not established.
     */
    if (state_ != kConnected) {
      // LOG_WARN << "Restart() must be used for reconnecting when passive closed by peer";
    }
    SetState(State::kDisconnected);
    retry_interval_ = INIT_RETRY_INTERVAL;

    StartRun();
  });
}

void Connector::Connect()
{
  loop_->AssertInThread();
  /* Avoid call StartRun when connection in connecting or connected */
  if (state_ != kDisconnected) return;

  int sockfd = sock::CreateNonBlockAndCloExecSocket(!serv_addr_.IsIpv4());
  // Poll to check connection if is established
  // If connection is established, we call CompleteConnect() to register write
  // callback, then write callback will call new_connection_callback_.
  // Otherwise, call Retry() to connect again.

  // In TCP socket,
  // the connect() initiates TCP's three-way shake.
  // It return only when connection is established or error occurrs.
  // \see UNP 4.3 connect Function
  auto ret = sock::Connect(sockfd, serv_addr_.ToSockaddr());

  auto saved_errno = (ret == 0) ? 0 : errno;

  // \see man connect
  switch (saved_errno) {
  case 0:
  case EINTR:
  case EINPROGRESS: // Connecting(Only for nonblocking)
  // case EISCONN: // Connected(continues harmlessly)
    CompleteConnect(sockfd);
    break;

  case EAGAIN: // For tcp, there are insufficient entries in the routing cache
  // case EADDRINUSE: // Local address is already in use
  case EADDRNOTAVAIL: // No port can be used(or enlarge port range?)
                      // Retry until there is a free port can be used
  case ENETUNREACH:   // ICMP "destination unreachable" from intermediate router
                      // Client kernel will send SYN in some time interval.
                      // If no response is received after some fixed amount of
                      // time, return this. Soft error
  case EHOSTUNREACH:  // Same with ENETUNREACH(In arch, no ENETUNREACH but
                      // EHOSTUNREACH in `man connect`)
  case ECONNREFUSED:  // Accept RST segments, unreachable address
                      // 1. the server is not listening in the specified port
                      // 2. peer aborts the connection
                      // 3. receive a segment from a nonexist connection(e.g.
                     // reboot and start connect, may receive a segment for last
                     // connection) Hard error \see UNP 4.3 connection Function
  case ETIMEDOUT: // no response to SYN segment
                  // NOTICE: client send SYN multiple in some time interval,
                  // e.g. 6s -> 24s.
                  // If no resposne is received after some fixed amount of time,
                  // Client kernel think this is a error.
    Retry(sockfd);
    break;

  default:
    LOG_SYSERROR << "Unexpected error in Connector::Connect()";
    sock::Close(sockfd);
  }
}

void Connector::CompleteConnect(int sockfd)
{
  KANON_ASSERT(state_ == State::kDisconnected && connect_,
               "Connection must be down and Stop isn't called(in loop)");

  /* Wait event occur */
  SetState(State::kConnecting);

  assert(!channel_);
  channel_ = kanon::make_unique<Channel>(loop_, sockfd);

  channel_->SetWriteCallback([this]() {
    int sockfd = channel_->GetFd();
    // NOTICE:
    // OUT maybe occurs with ERR(and HUP).
    // Therefore, the error callback is called before this,
    // use state to distinguish them.
    if (state_ == State::kConnecting) {
      ResetChannel();
      int err = sock::GetSocketError(sockfd);

      if (err) {
        // Fatal errors have handled in Connect()
        LOG_WARN << "SO_ERROR = " << err << " " << strerror_tl(err);
        Retry(sockfd);
      } else if (sock::IsSelfConnect(sockfd)) {
        // Self connection happend only when:
        // 1. The IP of client is same with server
        // 2. The port is a ephemeral port(selected by client kernel)
        // 3. Only occrurs in three-way shake initailization
        //
        // The self connection is so strange and hard to debug it.
        // We should refuse it.
        //
        // \see /proc/sys/net/ipv4/ip_local_port_range
        // \see https://github.com/pirDOL/kaka/blob/master/Miscellaneous/TCP-client-self-connect.md
        LOG_WARN << "self connect";
        // Discard the client address and retry
        // until self connection is skipped
        Retry(sockfd);
      } else {
        // new_connection_callback should be seted by client
        SetState(State::kConnected);

        if (connect_) {
          if (new_connection_callback_)
            new_connection_callback_(sockfd);
          else {
            LOG_WARN << "There is no new connection callback, cannot to create connection";
          }
          LOG_TRACE_KANON << "Connection is established successfully";
        } else {
          // Must after Stop() is called
          LOG_WARN << "Cannot to complete connect since user stop it";
          sock::Close(sockfd);
        }
      }
    }
  });

  channel_->SetErrorCallback([this]() {
    if (state_ == State::kConnecting) {
      int sockfd = channel_->GetFd();
      ResetChannel();
      int err = sock::GetSocketError(sockfd);
      if (err) {
        LOG_ERROR << "SO_ERROR = " << err << " " << strerror_tl(err);
      }

      Retry(sockfd);
    }
  });

  channel_->EnableWriting();
}

void Connector::Retry(int sockfd)
{
  loop_->AssertInThread();

  sock::Close(sockfd);
  SetState(kDisconnected);
  if (connect_) {
    double delay_sec =
        std::min<uint32_t>(retry_interval_, MAX_RETRY_INTERVAL) / 1000.0;

    LOG_TRACE_KANON << "Client will reconnect to " << serv_addr_.ToIpPort()
                    << " after " << delay_sec << " seconds";

    timer_ = loop_->RunAfter([this]() { StartRun(); }, delay_sec);

    retry_interval_ *= 2;
  }
}

void Connector::ResetChannel()
{
  // Ensure this is called in loop
  // This can be called in the phase2 or phase3
  loop_->AssertInThread();

  channel_->DisableAll();
  channel_->Remove();

  // \warning
  // In event handle phase now if called in phase2
  // So, you can't call it immediately
  loop_->QueueToLoop([this]() { channel_.reset(); });
}
