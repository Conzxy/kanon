#ifndef KANON_NET_EVENTLOOP_H
#define KANON_NET_EVENTLOOP_H

#include "util/noncopyable.h"
#include <vector>
#include <sys/types.h>
#include <atomic>

namespace kanon {

class Channel;

template<typename T>
class PollerBase;

class Poller;
class Epoller;

/*
 *			poll
 *		   ↗	↘
 *	functors ← handle events(including timerfd)
 */
template<typename T>
class EventLoopT : noncopyable {
public:
	EventLoopT();
	~EventLoopT() = default;
	
	void loop();
	
	void updateChannel(Channel* ch);
	void assertInThread() noexcept;
private:
	bool isLoopInThread() noexcept;
	void abortNotInThread() noexcept;
private:
	const pid_t ownerThreadId_;
	std::atomic<bool> looping_;
	std::atomic<bool> quit_;		

	std::vector<Channel> activeChannels_;
	PollerBase<T>* poller_;
};


#ifdef KANON_ENABLE_EPOLL
typedef EventLoopT<Epoller> EventLoop;
#else
typedef EventLoopT<Poller> EventLoop;
#endif

}

#endif // KANON_NET_EVENTLOOP_H
