#ifndef KANON_EVENTLOOP_THREAD_H
#define KANON_EVENTLOOP_THREAD_H

#include "EventLoop.h"
#include "kanon/thread/thread.h"
#include "kanon/thread/mutexlock.h"
#include "kanon/thread/condition.h"

namespace kanon {

/*
 * @brief used for IO thread
 */
class EventLoopThread : noncopyable {
public:
	EventLoopThread();
	
	// should be called by main thread	
	EventLoop* start();

	~EventLoopThread() = default;
private:
	EventLoop* loop_;
	
	MutexLock lock_;
	Condition cond_;
	Thread thr_;
};

} // namespace kanon

#endif // KANON_EVENTLOOP_THREAD_H
