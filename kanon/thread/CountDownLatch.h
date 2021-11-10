#ifndef KANON_COUNTDOWN_LATCH_H
#define KANON_COUNTDOWN_LATCH_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"

#include "MutexLock.h"
#include "kanon/thread/Condition.h"

// simple facility instead of pthread_barrier
// common usage:
// 1) main thread wait other thread complete their task
namespace kanon {

class CountDownLatch : noncopyable
{
public:
	explicit CountDownLatch(int count)
		: count_(count), mutex_(), cond_(mutex_)
	{ }

	void reset(int count) KANON_NOEXCEPT
	{ count_ = count; }

	int count() const KANON_NOEXCEPT
	{ return count_; }

	void wait()
	{
		MutexGuard guard(mutex_);
		if(count_ > 0)
			cond_.wait();
	}

	void countdown() KANON_NOEXCEPT
	{
		MutexGuard guard(mutex_);
		if(--count_ == 0)
			cond_.notifyAll();
	}
private:
	int count_;
	MutexLock mutex_;
	Condition cond_;
}; 

} // namespace kanon

#endif // KANON_COUNTDOWN_LATCH_H
