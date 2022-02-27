#ifndef KANON_NET_EPOLLER_H
#define KANON_NET_EPOLLER_H

#include "kanon/net/poller_base.h"
#include "kanon/time/time_stamp.h"

#include <sys/epoll.h>

namespace kanon {

/**
 * Epoll wrapper 
 * @note Take LT mode
 */
class Epoller final : public PollerBase {
public:
  typedef PollerBase Base;
    
  explicit Epoller(EventLoop* loop);
  ~Epoller() noexcept;

  TimeStamp Poll(int ms, ChannelVec& activeChannels) KANON_OVERRIDE;

  void UpdateChannel(Channel* ch) KANON_OVERRIDE;
  void RemoveChannel(Channel* ch) KANON_OVERRIDE;

private:
  void fillActiveChannels(int ev_nums, 
                          ChannelVec& activeChannels) noexcept;

  void UpdateEpollEvent(int op, Channel* ch) noexcept;

private:
  typedef struct epoll_event Event;
  typedef std::vector<Event> EventList;
  
  int epoll_fd_;
  EventList events_; // reuse to fill
};

} // namespace kanon

#endif // KANON_NET_EPOLLER_H
