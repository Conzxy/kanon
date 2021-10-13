#include "EventLoopThread.h"

using namespace kanon;

EventLoopThread::EventLoopThread()
	: lock_{}
	, cond_{ lock_ } 
	, thr_{ [this]() {
		// called in main thread
		EventLoop loop;

		{
			MutexGuard guard{ lock_ };
			loop_ = &loop;
			cond_.notify();
		}

		loop_->loop();
	} }
{ }

EventLoop*
EventLoopThread::start() {
	// called in IO thread
	thr_.start();

	{
		MutexGuard{ lock_ };
		while (loop_ == nullptr) {
			cond_.wait();
		}	
	}

	return loop_;
}



