#ifndef KANON_POLLER_H
#define KANON_POLLER_H

#include "kanon/util/noncopyable.h"
#include "kanon/time/time_stamp.h"
#include "kanon/util/type.h"
#include "kanon/net/channel.h"
#include "kanon/util/macro.h"
#include "kanon/net/event_loop.h"

#include <vector>

namespace kanon {

class EventLoop;

/*
 * @brief abstract base class of poll and epoll
 * @inherited by Poller and Epoller
 */
class PollerBase : noncopyable {
public:
  typedef std::vector<Channel*> ChannelVec;

  explicit PollerBase(EventLoop* loop)
    : loop_{ loop }
  { }
  
  virtual ~PollerBase() noexcept = default;

  // IO thread
  virtual TimeStamp Poll(int ms, ChannelVec& activeChannels) = 0;
  //TimeStamp Poll(int ms, ChannelVec& activeChannels) noexcept {
    //return static_cast<Poller*>(this)->Poll(ms, activeChannels);
  //}

  // add, delelte, update, search
  virtual void UpdateChannel(Channel* ch) = 0;
  //void UpdateChannel(Channel* ch) {
    //static_cast<Poller*>(this)->UpdateChannel(ch);
  //}
  
  virtual void RemoveChannel(Channel* ch) = 0;
  //void RemoveChannel(Channel* ch) {
    //static_cast<Poller*>(this)->RemoveChannel(ch);
  //}


  bool HasChannel(Channel* ch) {
    return channelMap_.find(ch->GetFd()) != channelMap_.end();
  }

protected:
  void AssertInThread() noexcept {
    loop_->AssertInThread();
  }

  typedef kanon::map<int, Channel*> ChannelMap;
  ChannelMap channelMap_;

private:
  EventLoop* loop_;
};

} // namespace kanon

#endif // KANON_POLLER_H
