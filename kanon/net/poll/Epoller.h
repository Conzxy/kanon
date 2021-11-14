#ifndef KANON_NET_EPOLLER_H
#define KANON_NET_EPOLLER_H

#include "kanon/net/PollerBase.h"
#include "kanon/time/TimeStamp.h"

#include <sys/epoll.h>

namespace kanon {

/**
 * @brief epoll wrapper 
 * @note take LT mode
 */
class Epoller final : public PollerBase {
public:
  typedef PollerBase Base;
    
    explicit Epoller(EventLoop* loop);
  ~Epoller() KANON_NOEXCEPT;

  TimeStamp poll(int ms, ChannelVec& activeChannels) KANON_OVERRIDE;

  void updateChannel(Channel* ch) KANON_OVERRIDE;
  void removeChannel(Channel* ch) KANON_OVERRIDE;

private:
    void fillActiveChannels(int ev_nums, 
                            ChannelVec& activeChannels) KANON_NOEXCEPT;

    void updateEpollEvent(int op, Channel* ch) KANON_NOEXCEPT;

private:
    typedef struct epoll_event Event;
  typedef std::vector<Event> EventList;
  
    int epoll_fd_;
    EventList events_; // reuse to fill
};

} // namespace kanon

#endif // KANON_NET_EPOLLER_H
