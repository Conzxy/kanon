#include "EventLoopThread.h"
#include "kanon/net/EventLoop.h"

using namespace kanon;

EventLoopThread::EventLoopThread(std::string const& name)
	: loop_{ nullptr }
	, lock_{}
	, cond_{ lock_ } 
	, thr_{ [this]() {
		// called in IO thread
	
		EventLoop loop{};
		{
			MutexGuard guard{ lock_ };
			// must allocate pointer in heap,
			// because stack of thread is private
			loop_ = &loop;
			cond_.notify();
		}
		
		// may quit before loop() be called
		loop_->loop();
		// FIXME: need mutex?
		//MutexGuard guard{ lock_ };
		loop_ = nullptr;
	}, name}
{ }

EventLoopThread::~EventLoopThread() KANON_NOEXCEPT {
	// if quit() is not called through pointer return from start
	// we should call quit explicitly	
	if (loop_ != nullptr) {
		loop_->quit();
		thr_.join(); // wait loop quiting
	}
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
	
	return loop_;
}



