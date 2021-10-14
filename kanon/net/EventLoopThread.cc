#include "EventLoopThread.h"
#include "kanon/net/EventLoop.h"

using namespace kanon;

EventLoopThread::EventLoopThread()
	: lock_{}
	, cond_{ lock_ } 
	, thr_{ [this]() {
		// called in IO thread

		{
			MutexGuard guard{ lock_ };
			// must allocate pointer in heap,
			// because stack of thread is private
			loop_ = kanon::make_unique<EventLoop>();
			cond_.notify();
		}

		loop_->loop();
	} }
{ }

EventLoopThread::~EventLoopThread() KANON_NOEXCEPT {
	assert(loop_);

	thr_.join();
}

EventLoop*
EventLoopThread::start() {
	thr_.start();

	{
		MutexGuard guard{ lock_ };
		while (loop_ == nullptr) {
			cond_.wait();
		}	
	}
	
	return getPointer(loop_);
}



