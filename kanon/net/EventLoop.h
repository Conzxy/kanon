#ifndef KANON_NET_EVENTLOOP_H
#define KANON_NET_EVENTLOOP_H

#include "kanon/util/noncopyable.h"
#include "kanon/thread/MutexLock.h"
#include "kanon/util/unique_ptr.h"
#include "kanon/net/timer/TimerQueue.h"
#include "kanon/util/macro.h"

#include <vector>
#include <sys/types.h>
#include <atomic>
#include <functional>

namespace kanon {

class Channel;
class PollerBase;

#define POLLTIME 10000 // 10 seconds

/*
 * @brief as follows:
 *		select/poll/epoll(event dispatching)
 *		   ↗	↘
 *	functors ← handle events(including timerfd)
 */
class EventLoop : noncopyable {
public:
	typedef std::function<void()> FunctorCallback;	
	typedef TimerQueue::TimerCallback TimerCallback;

	EventLoop();
	// note: since PollerBase is not exposed here
	// we should force out-of-line definition
	~EventLoop();
	
	/**
	 * @brief quit loop
	 * @note if not in thread, should call wakeup()
	 */
	void quit() KANON_NOEXCEPT;

	/**
	 * @brief start a event loop
	 */
	void loop();

	/**
	 * @tag asynchronous call
	 * @brief run functor in the loop \n
	 *		  1) if not in thread, should append to functors_ \n
	 *		  2) otherwise, call it immediately \n
	 */
	void runInLoop(FunctorCallback);

	/**
	 * @tag asynchronous call
	 * @brief append functor
	 * @note used in multithread environment
	 */
	void queueToLoop(FunctorCallback);
	
	/**
	 * @brief XXXChannel is just forward to poll to do something
	 */
	void removeChannel(Channel* ch);
	void updateChannel(Channel* ch);
	void hasChannel(Channel* ch);

	/**
	 * @brief call all functors in functors_(better name: callable)
	 */
	void callFunctors();

	/**
	 * @brief timer API
	 */
	TimerId runAt(TimerCallback cb, TimeStamp expiration) {
		return timer_queue_.addTimer(std::move(cb), expiration, 0.0);
	}

	TimerId runAfter(TimerCallback cb, double delay) {
		return this->runAt(std::move(cb), addTime(TimeStamp::now(), delay));
	}

	TimerId runEvery(TimerCallback cb, TimeStamp expiration, double interval) {
		return timer_queue_.addTimer(std::move(cb), expiration, interval);
	}

	TimerId runEvery(TimerCallback cb, double interval) {
		return this->runEvery(std::move(cb), addTime(TimeStamp::now(), interval), interval);
	}

	void cancelTimer(TimerId const& timer_id) {
		timer_queue_.cancelTimer(timer_id);
	}
	//void cancelTimer(TimerId id);

	/**
	 * @brief maintain invariant for "one loop per thread" policy
	 * @note although release version, it also work
	 */
	void assertInThread() KANON_NOEXCEPT;
	bool isLoopInThread() KANON_NOEXCEPT;

private:
	/**
	 * @brief write some data to kernel buffer of eventfd,
	 *		  to avoid poll block for long time
	 *		  ((e)poll have to handle and return at once)
	 */
	void wakeup() KANON_NOEXCEPT;

	/*
	 * @brief read callback of eventfd
	 */
	void evRead() KANON_NOEXCEPT;
	
	void abortNotInThread() KANON_NOEXCEPT;
private:
	const pid_t ownerThreadId_;
	std::unique_ptr<PollerBase> poller_;

	// FIXME: atomic bool
	bool looping_;
	bool quit_;		
	bool callingFunctors_;
	
	int evfd_;
	std::unique_ptr<Channel> ev_channel_;

	MutexLock lock_;

	std::vector<FunctorCallback> functors_;
	std::vector<Channel*> activeChannels_;
	TimerQueue timer_queue_;
};

}


#endif // KANON_NET_EVENTLOOP_H
