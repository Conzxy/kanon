#ifndef KANON_NET_POLL_POLLER_H
#define KANON_NET_POLL_POLLER_H

#include "kanon/net/PollerBase.h"
#include <poll.h>

namespace kanon {

class Poller final : public PollerBase {
	typedef PollerBase Base;
	typedef std::vector<struct pollfd> PollfdVec;
public:
	using Base::Base;

	TimeStamp poll(int ms, ChannelVec& activeChannels) KANON_NOEXCEPT KANON_OVERRIDE;
	void updateChannel(Channel* ch) KANON_OVERRIDE;
	void removeChannel(Channel* ch) KANON_OVERRIDE;

private:
	PollfdVec pollfds_;	
};


} // namespace kanon

#endif // KANON_NET_POLL_POLLER_H
