#ifndef KANON_EVENTLOOP_THREAD_H
#define KANON_EVENTLOOP_THREAD_H

#include "EventLoop.h"
#include "kanon/thread/thread.h"
#include "kanon/thread/mutexlock.h"
#include "kanon/thread/condition.h"

namespace kanon {
	
class EventLoopThread : noncopyable {
public:
	EventLoopThread();
	
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
