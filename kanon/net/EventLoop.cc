#include "kanon/net/EventLoop.h"
#include "kanon/thread/current-thread.h"
#include "kanon/log/Logger.h"
#include "kanon/net/Channel.h"
#include "kanon/time/TimeStamp.h"
#include "kanon/util/macro.h"
#include "kanon/net/poll/Poller.h"
#include "kanon/net/poll/Epoller.h"
#include "kanon/net/macro.h"

#include <assert.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace kanon {

namespace detail {

/**
 * @brief event fd API
 */
static int createEventfd() noexcept {
	int evfd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

	LOG_TRACE << "eventfd " << evfd << " created";

	if (evfd < 0) {
		LOG_SYSERROR << "eventfd() error occurred";
	}

	return evfd;
}

static void readEventfd(int evfd) noexcept {
	uint64_t dummy;
	if (sizeof dummy != ::read(evfd, &dummy, sizeof dummy))
		LOG_SYSERROR << "readEventfd() error occurred";
}

static void writeEventfd(int evfd) noexcept {
	uint64_t dummy = 0;
	if (sizeof dummy != ::write(evfd, &dummy, sizeof dummy))
		LOG_SYSERROR << "writeEventfd() error occurred";
}

} // namespace detail

EventLoop::EventLoop()
	: ownerThreadId_{ CurrentThread::t_tid }
#ifdef ENABLE_EPOLL
	, poller_{ kanon::make_unique<Epoller>(this) }
#else
	, poller_{ kanon::make_unique<Poller>(this) }
#endif
	, looping_{ false }
	, quit_{ false }
	, callingFunctors_{ false }
	, evfd_{ detail::createEventfd() }
	, ev_channel_{ kanon::make_unique<Channel>(this, evfd_) }
	, timer_queue_{ this }
{ 
	ev_channel_->setReadCallback([this](TimeStamp receive_time){
		LOG_TRACE << "event receive_time: " << receive_time.toFormattedString(true);
		this->evRead();
	});

	ev_channel_->setWriteCallback([this](){
		this->wakeup();
	});

	ev_channel_->enableReading();

	LOG_TRACE << "EventLoop " << this 
			  << " created";
}

EventLoop::~EventLoop()
{ }

void EventLoop::loop() {
	assert(!looping_);
	assert(!quit_);
	this->assertInThread();
	
	looping_ = true;
	
	LOG_TRACE << "EventLoop " << this << " loop start";
	
	while (!quit_) {
		auto receive_time = poller_->poll(POLLTIME, activeChannels_);

		for (auto& channel : activeChannels_) {
			channel->handleEvents(receive_time);
		}
	
		callFunctors();

		activeChannels_.clear();
	}

	LOG_TRACE << "EventLoop " << this << " loop stop";
	
	looping_ = false;
}

void EventLoop::runInLoop(FunctorCallback cb) {
	if (isLoopInThread()) {
		cb();	
	} else {
		this->queueToLoop(std::move(cb));
	}
}

void EventLoop::queueToLoop(FunctorCallback cb) {
	{
		MutexGuard dummy(lock_);
		functors_.emplace_back(std::move(cb));
	}

	if (! isLoopInThread() ||
		callingFunctors_) {
		wakeup();
	}
}

void EventLoop::updateChannel(Channel* ch) {
	poller_->updateChannel(ch);
}

void EventLoop::removeChannel(Channel* ch) {
	poller_->removeChannel(ch);
}

void EventLoop::hasChannel(Channel* ch) {
	poller_->hasChannel(ch);
}

TimerId EventLoop::runAt(TimerCallback cb, TimeStamp expiration) {
	return timer_queue_.addTimer(std::move(cb), expiration, 0.0);
}

TimerId EventLoop::runAfter(TimerCallback cb, double delay) {
	return this->runAt(std::move(cb), addTime(TimeStamp::now(), delay));
}

TimerId EventLoop::runEvery(TimerCallback cb, TimeStamp expiration, double interval) {
	return timer_queue_.addTimer(std::move(cb), expiration, interval);
}

TimerId EventLoop::runEvery(TimerCallback cb, double interval) {
	return this->runEvery(std::move(cb), addTime(TimeStamp::now(), interval), interval);
}

void EventLoop::callFunctors() {
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

void EventLoop::assertInThread() noexcept {
	if (!this->isLoopInThread())
		this->abortNotInThread();
}

bool EventLoop::isLoopInThread() noexcept {
	return CurrentThread::t_tid == ownerThreadId_;

}

void EventLoop::abortNotInThread() noexcept {
	LOG_FATAL << "You Should obey one loop per thread policy";
}

void EventLoop::evRead() KANON_NOEXCEPT {
	detail::readEventfd(evfd_);
}

void EventLoop::wakeup() KANON_NOEXCEPT {
	detail::writeEventfd(evfd_);
}

void EventLoop::quit() noexcept {
	quit_ = true;
	
	if (! this->isLoopInThread())
		this->wakeup();
}

} // namespace kanon

