#ifndef KANON_POLLER_H
#define KANON_POLLER_H

#include "kanon/util/noncopyable.h"
#include "kanon/time/TimeStamp.h"
#include <vector>
#include <map>

namespace kanon {

class Channel;
class EventLoop;

extern int POLLTIMEOUT;

/*
 * @brief abstract base class of poll and epoll
 * @inherited by Poller and Epoller
 */
template<typename Poller>
class PollerBase : noncopyable {
public:
	typedef std::vector<Channel> ChannelVec;

	explicit PollerBase(EventLoop* loop)
		: loop_{ loop }
	{ }

	// IO thread
	TimeStamp poll(int ms, ChannelVec& activeChannels) noexcept {
		static_cast<Poller*>(this)->poll(ms, activeChannels);
	}

	// add, delelte, update, search
	void updateChannel(Channel* ch) {
		static_cast<Poller*>(this)->updateChannel(ch);
	}

	void removeChannel(Channel* ch) {
		static_cast<Poller*>(this)->removeChannel(ch);
	}

	void hasChannel(Channel* ch) {
		static_cast<Poller*>(this)->hasChannel(ch);
	}

protected:
	typedef std::map<int, Channel*> ChannelMap;
	ChannelMap channelMap_;

private:
	EventLoop* loop_;
};

} // namespace kanon

#endif // KANON_POLLER_H
