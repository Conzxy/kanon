#ifndef KANON_COUNTDOWN_LATCH_H
#define KANON_COUNTDOWN_LATCH_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"

#include "kanon/thread/mutex_lock.h"
#include "kanon/thread/condition.h"

// simple facility instead of pthread_barrier
// common usage:
// 1) main thread wait other thread complete their task
namespace kanon {

class CountDownLatch : noncopyable
{
public:
  CountDownLatch()
    : CountDownLatch(0)
  { }

  explicit CountDownLatch(int count)
    : count_(count), mutex_(), cond_(mutex_)
  { }

  void Reset(int count) noexcept
  { count_ = count; }

  int GetCount() const noexcept
  { return count_; }

  void Wait()
  {
    MutexGuard guard(mutex_);
    if(count_ > 0)
      cond_.Wait();
  }

  void Countdown() noexcept
  {
    MutexGuard guard(mutex_);
    if(--count_ == 0)
      cond_.NotifyAll();
  }
private:
  int count_;
  MutexLock mutex_;
  Condition cond_;
}; 

} // namespace kanon

#endif // KANON_COUNTDOWN_LATCH_H
