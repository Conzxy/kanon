#ifndef KANON_NET_EVENTLOOP_H
#define KANON_NET_EVENTLOOP_H

#include "kanon/util/noncopyable.h"
#include "kanon/thread/mutexlock.h"
#include "kanon/util/unique_ptr.h"
#include "kanon/net/timer/TimerQueue.h"

#include <vector>
#include <sys/types.h>
#include <atomic>
#include <functional>

namespace kanon {

class Channel;

template<typename T>
class PollerBase;

#define POLLTIME 1000

/*
 */
template<typename T>
class EventLoopT : noncopyable {
public:
	typedef std::function<void()> FunctorCallback;	
	typedef TimerQueue::TimerCallback TimerCallback;

	EventLoopT();
	~EventLoopT() = default;
	
	/**
	 * @brief quit loop
	 * @note if not in thread, should call wakeup()
	 */
	void quit() noexcept;

	/**
	 * @brief start a event loop
	 *		select/poll/epoll(event dispatching)
	 *		   ↗	↘
	 *	functors ← handle events(including timerfd)
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
	TimerId runAt(TimerCallback cb, TimeStamp expiration);
	TimerId runAfter(TimerCallback cb, double delay);
	TimerId runEvery(TimerCallback cb, TimeStamp expiration, double interval);
	TimerId runEvery(TimerCallback cb, double interval);

	//void cancelTimer(TimerId id);

	/**
	 * @brief maintain invariant for "one loop per thread" policy
	 * @note although release version, it also work
	 */
	void assertInThread() noexcept;

private:
	/**
	 * @brief write some data to kernel buffer of eventfd,
	 *		  to avoid poll block for long time
	 *		  ((e)poll have to handle and return at once)
	 */
	void wakeup();

	/*
	 * @brief read callback of eventfd
	 */
	void evRead();
	
	bool isLoopInThread() noexcept;
	void abortNotInThread() noexcept;
private:
	const pid_t ownerThreadId_;

	// FIXME: atomic bool
	bool looping_;
	bool quit_;		
	bool callingFunctors_;
	
	int evfd_;
	std::unique_ptr<Channel> ev_channel_;

	MutexLock lock_;

	TimerQueue timer_queue_;

	std::vector<FunctorCallback> functors_;
	std::vector<Channel*> activeChannels_;
	std::unique_ptr<PollerBase<T>> poller_;
};


}

#include "EventLoop_impl.h"
#include "type.h"

#endif // KANON_NET_EVENTLOOP_H
