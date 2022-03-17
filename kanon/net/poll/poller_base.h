#ifndef KANON_POLLER_H
#define KANON_POLLER_H

#include <vector>

#include "kanon/util/noncopyable.h"
#include "kanon/util/time_stamp.h"
#include "kanon/util/type.h"
#include "kanon/util/macro.h"
#include "kanon/net/channel.h"
#include "kanon/net/event_loop.h"

namespace kanon {

class EventLoop;

/**
 * @brief abstract base class of poll and epoll
 * @inherited by Poller and Epoller
 */
class PollerBase : noncopyable {
public:
  using ChannelVec = std::vector<Channel*>;

  explicit PollerBase(EventLoop* loop)
    : loop_{ loop }
  { }
  
  virtual ~PollerBase() noexcept = default;

  // IO thread
  virtual TimeStamp Poll(int ms, ChannelVec& activeChannels) = 0;

  // add, delelte, update, search
  virtual void UpdateChannel(Channel* ch) = 0;
  virtual void RemoveChannel(Channel* ch) = 0;

protected:
  void AssertInThread() noexcept {
    loop_->AssertInThread();
  }

private:
  EventLoop* loop_;
};

} // namespace kanon

#endif // KANON_POLLER_H
