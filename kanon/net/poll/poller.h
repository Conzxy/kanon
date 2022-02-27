#ifndef KANON_NET_POLL_POLLER_H
#define KANON_NET_POLL_POLLER_H

#include "kanon/net/poller_base.h"
#include <poll.h>

namespace kanon {

class Poller final : public PollerBase {
  typedef PollerBase Base;
  typedef std::vector<struct pollfd> PollfdVec;
public:
  using Base::Base;

  TimeStamp Poll(int ms, ChannelVec& activeChannels) noexcept KANON_OVERRIDE;
  void UpdateChannel(Channel* ch) KANON_OVERRIDE;
  void RemoveChannel(Channel* ch) KANON_OVERRIDE;

private:
  PollfdVec pollfds_;  
};


} // namespace kanon

#endif // KANON_NET_POLL_POLLER_H
