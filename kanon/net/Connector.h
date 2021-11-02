#ifndef KANON_NET_CONNECTOR_H
#define KANON_NET_CONNECTOR_H

#include "kanon/util/noncopyable.h"
#include "kanon/net/InetAddr.h"
#include "kanon/util/macro.h"
#include "kanon/util/unique_ptr.h"

#include <functional>
#include <atomic>

namespace kanon {

class EventLoop;
class Channel;

class Connector : noncopyable {
  enum class State {
    kConnecting,
    kConnected,
    kDisconnected,
  };

  typedef std::function<void(int sockfd)> NewConnectionCallback;  

public:
  Connector(EventLoop* loop,
            InetAddr const& servAddr);

  ~Connector() KANON_NOEXCEPT;

  // Call connect() to connect server in servAddr_
  // If connect() fails, continue call this till connect to peer
  // @note Not thread safe, but in loop
  void start() KANON_NOEXCEPT;

  // stop retry connect to peer when fials infinitely
  // @note Not thread safe, but in loop
  void stop() KANON_NOEXCEPT;

  // restart connector to connect to peer
  // used as close callback that will be called
  // when closed by peer passively
  // @note Not thread safe, but in loop
  void restart() KANON_NOEXCEPT;

  void setNewConnectionCallback(NewConnectionCallback cb) KANON_NOEXCEPT
  { new_connection_callback_ = std::move(cb); }

private:
  void setState(State s) KANON_NOEXCEPT { state_ = s; }

  void connect() KANON_NOEXCEPT;
  void completeConnect(int sockfd) KANON_NOEXCEPT;
  void retry(int sockfd) KANON_NOEXCEPT;
  int removeAndResetChannel() KANON_NOEXCEPT;

  EventLoop* loop_;
  InetAddr servAddr_;

  std::atomic<bool> connect_; // control if continues to connect
  State state_;
  std::unique_ptr<Channel> channel_;
  uint32_t retryInterval_;  

  NewConnectionCallback new_connection_callback_;
};

} // namespace kanon

#endif // KANON_NET_CONNECTOR_H
