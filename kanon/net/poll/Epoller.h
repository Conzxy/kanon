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
class Epoller final : public PollerBase<Epoller> {
public:
	typedef PollerBase<Epoller> Base;
	
	using Base::Base;

	TimeStamp poll(int ms, ChannelVec& activeChannels) KANON_NOEXCEPT;

	void updateChannel(Channel* ch);
	void removeChannel(Channel* ch);

private:
	typedef std::vector<struct epoll_event
	
};

} // namespace kanon

#endif // KANON_NET_EPOLLER_H
