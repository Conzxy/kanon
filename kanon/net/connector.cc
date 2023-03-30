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

Connector::~Connector() KANON_NOEXCEPT
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
  if (timer_) loop_->CancelTimer(*timer_);

  if (state_ == State::kConnecting) {
    // Only be called when connecting,
    // interrupt reconnecting to peer
    FdType sockfd = channel_->GetFd();
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
      // LOG_WARN << "Restart() must be used for reconnecting when passive
      // closed by peer";
    }
    SetState(State::kDisconnected);
    retry_interval_ = INIT_RETRY_INTERVAL;

    StartRun();
  });
}

void Connector::Retry(FdType sockfd)
{
  loop_->AssertInThread();

  sock::Close(sockfd);
  SetState(kDisconnected);
  if (connect_) {
    double delay_sec =
        std::min<uint32_t>(retry_interval_, MAX_RETRY_INTERVAL) / 1000.0;

    LOG_TRACE_KANON << "Client will reconnect to " << serv_addr_.ToIpPort()
                    << " after " << delay_sec << " seconds";

    timer_ = loop_->RunAfter(
        [this]() {
          StartRun();
        },
        delay_sec);

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
  loop_->QueueToLoop([this]() {
    channel_.reset();
  });
}
