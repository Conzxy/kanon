#ifndef KANON_NET_POLL_POLLER_H
#define KANON_NET_POLL_POLLER_H

#include <poll.h>
#include <map>

#include "kanon/net/poll/poller_base.h"

namespace kanon {

class Poller final : public PollerBase {
public:
  explicit Poller(EventLoop* loop);

  TimeStamp Poll(int ms, ChannelVec& activeChannels) noexcept KANON_OVERRIDE;
  void UpdateChannel(Channel* ch) KANON_OVERRIDE;
  void RemoveChannel(Channel* ch) KANON_OVERRIDE;

private:
  /**
   * struct pollfd {
   *  int fd;
   *  short events;
   *  short revents;
   * };
   */
  std::vector<struct pollfd> pollfds_;  
  std::map<int, Channel*> channels_map_;
};


} // namespace kanon

#endif // KANON_NET_POLL_POLLER_H
