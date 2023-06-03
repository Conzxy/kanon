#ifndef KANON_COUNTDOWN_LATCH_H
#define KANON_COUNTDOWN_LATCH_H

#include <cassert>
#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/thread/condition.h"

// simple facility instead of pthread_barrier
// common usage:
// 1) main thread wait other thread complete their task
namespace kanon {

class CountDownLatch : noncopyable {
 public:
  CountDownLatch()
    : CountDownLatch(0)
  {
  }

  explicit CountDownLatch(int count)
    : count_(count)
    , mutex_()
    , cond_(mutex_)
  {
  }

  void Reset(int count) KANON_NOEXCEPT
  {
    MutexGuard guard(mutex_);
    count_ = count;
  }

  void ResetAndWait(int count) KANON_NOEXCEPT
  {
    MutexGuard guard(mutex_);
    count_ = count;
    if (count_ > 0) cond_.Wait();
  }

  void Add(int count) KANON_NOEXCEPT
  {
    MutexGuard guard(mutex_);
    count_ += count;
  }

  void AddAndWait(int count) KANON_NOEXCEPT
  {
    MutexGuard guard(mutex_);
    count_ += count;
    if (count_ > 0) cond_.Wait();
  }

  void Incr() KANON_NOEXCEPT
  {
    MutexGuard guard(mutex_);
    ++count_;
  }

  void IncrAndWait() KANON_NOEXCEPT
  {
    MutexGuard guard(mutex_);
    ++count_;
    if (count_ > 0) cond_.Wait();
  }

  KANON_DEPRECATED_ATTR void Minus(int count) KANON_NOEXCEPT
  {
    MutexGuard guard(mutex_);
    count_ -= count;
  }

  KANON_DEPRECATED_ATTR void Decr() KANON_NOEXCEPT
  {
    MutexGuard guard(mutex_);
    --count_;
  }

  int GetCount() const KANON_NOEXCEPT
  {
    MutexGuard guard(mutex_);
    return count_;
  }

  void Wait()
  {
    MutexGuard guard(mutex_);
    if (count_ > 0) cond_.Wait();
  }

  void Countdown() KANON_NOEXCEPT
  {
    MutexGuard guard(mutex_);
    --count_;
    if (count_ == 0) {
      cond_.NotifyAll();
    }
  }

 private:
  int count_;
  mutable MutexLock mutex_;
  Condition cond_ GUARDED_BY(mutex_);
};

} // namespace kanon

#endif // KANON_COUNTDOWN_LATCH_H
