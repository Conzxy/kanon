#ifndef KANON_NET_EVENTLOOP_IMPL_H
#define KANON_NET_EVENTLOOP_IMPL_H

#include "kanon/thread/current-thread.h"
#include "kanon/log/Logger.h"
#include "kanon/net/Channel.h"
#include "kanon/util/macro.h"

#include <assert.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace kanon {

namespace detail {

/**
 * @brief event fd API
 */
inline int createEventfd() noexcept {
	int evfd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

	LOG_TRACE << "eventfd " << evfd << " created";

	if (evfd < 0) {
		LOG_SYSERROR << "eventfd() error occurred";
	}

	return evfd;
}

inline void readEventfd(int evfd) noexcept {
	uint64_t dummy;
	if (sizeof dummy != ::read(evfd, &dummy, sizeof dummy))
		LOG_SYSERROR << "readEventfd() error occurred";
}

inline void writeEventfd(int evfd) noexcept {
	uint64_t dummy = 0;
	if (sizeof dummy != ::write(evfd, &dummy, sizeof dummy))
		LOG_SYSERROR << "writeEventfd() error occurred";
}

} // namespace detail

template<typename T>
EventLoopT<T>::EventLoopT()
	: ownerThreadId_{ CurrentThread::t_tid }
	, poller_{ kanon::make_unique<T>(this) }
	, looping_{ false }
	, quit_{ false }
	, callingFunctors_{ false }
	, evfd_{ detail::createEventfd() }
	, ev_channel_{ kanon::make_unique<Channel>(this, evfd_) }
	, timer_queue_{ this }
{ 
	ev_channel_->set_read_callback([this](){
		this->evRead();
	});

	ev_channel_->set_write_callback([this](){
		this->wakeup();
	});

	ev_channel_->enableReading();

	LOG_TRACE << "EventLoop " << this 
			  << " created";
}

template<typename T>
void EventLoopT<T>::loop() {
	assert(!looping_);
	assert(!quit_);
	this->assertInThread();
	
	looping_ = true;
	
	LOG_TRACE << "EventLoop " << this << " loop start";
	
	while (!quit_) {
		poller_->poll(POLLTIME, activeChannels_);

		for (auto& channel : activeChannels_) {
			channel->handleEvents();
		}
	
		callFunctors();

		activeChannels_.clear();
	}

	LOG_TRACE << "EventLoop " << this << " loop stop";
	
	looping_ = false;
}

template<typename T>
void EventLoopT<T>::runInLoop(FunctorCallback cb) {
	if (isLoopInThread()) {
		cb();	
	} else {
		this->queueToLoop(std::move(cb));
	}
}

template<typename T>
void EventLoopT<T>::queueToLoop(FunctorCallback cb) {
	{
		MutexGuard dummy(lock_);
		functors_.emplace_back(std::move(cb));
	}

	if (! isLoopInThread() ||
		callingFunctors_) {
		wakeup();
	}
}

template<typename T>
inline void EventLoopT<T>::updateChannel(Channel* ch) {
	poller_->updateChannel(ch);
}

template<typename T>
inline void EventLoopT<T>::removeChannel(Channel* ch) {
	poller_->removeChannel(ch);
}

template<typename T>
inline void EventLoopT<T>::hasChannel(Channel* ch) {
	poller_->hasChannel(ch);
}

template<typename T>
inline TimerId EventLoopT<T>::runAt(TimerCallback cb, TimeStamp expiration) {
	return timer_queue_.addTimer(std::move(cb), expiration, 0.0);
}
template<typename T>
inline TimerId EventLoopT<T>::runAfter(TimerCallback cb, double delay) {
	return this->runAt(std::move(cb), addTime(TimeStamp::now(), delay));
}

template<typename T>
inline TimerId EventLoopT<T>::runEvery(TimerCallback cb, TimeStamp expiration, double interval) {
	return timer_queue_.addTimer(std::move(cb), expiration, interval);
}

template<typename T>
inline TimerId EventLoopT<T>::runEvery(TimerCallback cb, double interval) {
	return this->runEvery(std::move(cb), addTime(TimeStamp::now(), interval), interval);
}

template<typename T>
inline void EventLoopT<T>::callFunctors() {
	decltype(functors_) functors;
	{
		MutexGuard dummy{ lock_ };
		functors.swap(functors_);
	}	

	callingFunctors_ = true;
	// FIXME: auto& better?
	for (auto const& func : functors) {
		try {
			func();
		} catch(std::exception const& ex) {
			LOG_ERROR << "std::exception caught in callFunctors()"
					  << "what(): " << ex.what();	
			KANON_RETHROW;
			callingFunctors_ = false;
		} catch(...) {
			LOG_ERROR << "some exception caught in callFunctors()";
			KANON_RETHROW;
			callingFunctors_ = false;
		}
	}

	callingFunctors_ = false;
}

template<typename T>
inline void EventLoopT<T>::assertInThread() noexcept {
	if (!this->isLoopInThread())
		this->abortNotInThread();
}

template<typename T>
inline bool EventLoopT<T>::isLoopInThread() noexcept {
	return CurrentThread::t_tid == ownerThreadId_;

}

template<typename T>
inline void EventLoopT<T>::abortNotInThread() noexcept {
	LOG_ERROR << "You Should obey one loop per thread policy";
}

template<typename T>
inline void EventLoopT<T>::evRead() KANON_NOEXCEPT {
	detail::readEventfd(evfd_);
}

template<typename T>
inline void EventLoopT<T>::wakeup() KANON_NOEXCEPT {
	detail::writeEventfd(evfd_);
}

template<typename T>
inline void EventLoopT<T>::quit() noexcept {
	quit_ = true;
	
	if (! this->isLoopInThread())
		this->wakeup();
}

} // namespace kanon

#include "kanon/net/poll/Poller.h"
#include "kanon/net/poll/Epoller.h"

#endif // KANON_NET_EVENTLOOP_IMPL_H
