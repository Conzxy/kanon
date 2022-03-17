#ifndef KANON_NET_EPOLLER_H
#define KANON_NET_EPOLLER_H

#include <sys/epoll.h>

#include "kanon/util/time_stamp.h"

#include "kanon/net/poll/poller_base.h"

namespace kanon {

/**
 * Epoll wrapper 
 * @note Take LT mode
 */
class Epoller final : public PollerBase {
public:
  explicit Epoller(EventLoop* loop);
  ~Epoller() noexcept;

  TimeStamp Poll(int ms, ChannelVec& activeChannels) KANON_OVERRIDE;

  void UpdateChannel(Channel* ch) KANON_OVERRIDE;
  void RemoveChannel(Channel* ch) KANON_OVERRIDE;

private:
  void FillActiveChannels(int ev_nums, 
                          ChannelVec& activeChannels) noexcept;

  void UpdateEpollEvent(int op, Channel* ch) noexcept;

private:
  // typedef union epoll_data {
  //  void *ptr;
  //  int fd;
  //  uint32_t u32;
  //  uint64_t u64;
  // } epoll_data_t;
  //
  // struct epoll_event {
  //   uint32_t events; // Epoll events
  //   epoll_data_t data; // user data variable
  // };
  using Event = struct epoll_event;
  
  int epoll_fd_;
  std::vector<Event> events_; // reuse to fill
};

} // namespace kanon

#endif // KANON_NET_EPOLLER_H
