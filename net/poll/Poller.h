#ifndef KANON_NET_POLL_POLLER_H
#define KANON_NET_POLL_POLLER_H

#include "kanon/net/PollerBase.h"
#include <poll.h>

namespace kanon {

class Poller final : public PollerBase<Poller> {
	typedef PollerBase<Poller> Base;
	typedef std::vector<struct pollfd> PollfdVec;
public:
	using Base::Base;

	TimeStamp poll(int ms, ChannelVec& activeChannels) noexcept;
	void updateChannel(Channel* ch);
	void removeChannel(Channel* ch);
	void hasChannel(Channel* ch);

private:
	PollfdVec pollfds_;	
};


} // namespace kanon

#endif // KANON_NET_POLL_POLLER_H
