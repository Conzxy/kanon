#ifndef KANON_NET_CONNECTOR_H
#define KANON_NET_CONNECTOR_H

#include <functional>
#include <atomic>

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/ptr.h"
#include "kanon/net/timer/timer_id.h"

#include "kanon/net/inet_addr.h"

namespace kanon {

class EventLoop;
class Channel;

class Connector : public std::enable_shared_from_this<Connector>, noncopyable {
  enum class State {
    kConnecting,
    kConnected,
    kDisconnected,
  };

  typedef std::function<void(int sockfd)> NewConnectionCallback;  

public:
  Connector(EventLoop* loop,
            InetAddr const& servAddr);

  ~Connector() noexcept;

  // Call Connect() to connect server in servAddr_
  // If Connect() fails, continue call this till connect to peer
  // \note Not thread safe, but in loop
  void StartRun() noexcept;

  // stop retry connect to peer when fials infinitely
  // \note Not thread safe, but in loop
  void Stop() noexcept;

  // restart connector to connect to peer
  // used as close callback that will be called
  // when closed by peer passively
  // \note Not thread safe, but in loop
  void Restrat() noexcept;

  void SetNewConnectionCallback(NewConnectionCallback cb) noexcept
  { new_connection_callback_ = std::move(cb); }

private:
  void setState(State s) noexcept { state_ = s; }

  void Connect() noexcept;
  void CompleteConnect(int sockfd) noexcept;
  void Retry(int sockfd) noexcept;
  int RemoveAndResetChannel() noexcept;

  void StopInLoop() noexcept;

  EventLoop* loop_;
  InetAddr servAddr_;

  std::atomic<bool> connect_; // control if continues to connect
  State state_;
  std::unique_ptr<Channel> channel_;
  uint32_t retryInterval_;  

  NewConnectionCallback new_connection_callback_;
  TimerId timer_;
};

} // namespace kanon

#endif // KANON_NET_CONNECTOR_H
