#ifndef KANON_POLLER_H
#define KANON_POLLER_H

#include "kanon/util/noncopyable.h"
#include "kanon/time/TimeStamp.h"
#include "kanon/util/type.h"
#include "kanon/net/Channel.h"
#include "kanon/util/macro.h"

#include <vector>

namespace kanon {

template<typename T>
class EventLoopT;

/*
 * @brief abstract base class of poll and epoll
 * @inherited by Poller and Epoller
 */
template<typename Poller>
class PollerBase : noncopyable {
public:
	typedef std::vector<Channel*> ChannelVec;

	explicit PollerBase(EventLoopT<Poller>* loop)
		: loop_{ loop }
	{ }

	// IO thread
	TimeStamp poll(int ms, ChannelVec& activeChannels) noexcept {
		return static_cast<Poller*>(this)->poll(ms, activeChannels);
	}

	// add, delelte, update, search
	void updateChannel(Channel* ch) {
		static_cast<Poller*>(this)->updateChannel(ch);
	}

	void removeChannel(Channel* ch) {
		static_cast<Poller*>(this)->removeChannel(ch);
	}


	bool hasChannel(Channel* ch) {
		return channelMap_.find(ch->fd()) != channelMap_.end();
	}

protected:
	void assertInThread() noexcept {
		loop_->assertInThread();
	}

	typedef kanon::map<int, Channel*> ChannelMap;
	ChannelMap channelMap_;

private:
	EventLoopT<Poller>* loop_;
};

} // namespace kanon

#endif // KANON_POLLER_H
