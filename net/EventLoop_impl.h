#ifndef KANON_NET_EVENTLOOP_IMPL_H
#define KANON_NET_EVENTLOOP_IMPL_H

#include "kanon/net/EventLoop.h"
#include <assert.h>
#include "thread/current-thread.h"
#include "PollerBase.h"
#include "log/Logger.h"

namespace kanon {

template<typename T>
void EventLoopT<T>::loop() {
	assert(!looping_);
	assert(isLoopInThread());
	looping_ = true;
	
	LOG_TRACE << "EventLoop::loop() start";
	// poll
	
	LOG_TRACE << "EventLoop::loop() end";

	looping_ = false;
}

template<typename T>
inline void EventLoopT<T>::updateChannel(Channel* ch) {
	poller_->updateChannel(ch);
}

template<typename T>
inline void EventLoopT<T>::assertInThread() noexcept {
	if (!isLoopInThread())
		abortNotInThread();
}

template<typename T>
inline bool EventLoopT<T>::isLoopInThread() noexcept {
	return CurrentThread::t_tid == ownerThreadId_;
}

template<typename T>
inline void abortNotInThread() noexcept {
	LOG_ERROR << "You Should obey one loop per thread";
}

} // namespace kanon

#endif // KANON_NET_EVENTLOOP_IMPL_H
