#ifndef KANON_TIMER_TIMER_H
#define KANON_TIMER_TIMER_H

#include <functional>

#include "kanon/util/time_stamp.h"
#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/thread/atomic_counter.h"

namespace kanon {

class TimerQueue;

//! \addtogroup timer
//!@{

/**
 * \brief
 * Encapsulation of itimespec
 */
class KANON_NET_NO_API Timer : noncopyable {
 public:
  // friend class TimerQueue;
  typedef std::function<void()> TimerCallback;

  KANON_INLINE Timer(TimerCallback cb, TimeStamp expiration, double interval)
    : callback_{std::move(cb)}
    , expiration_{expiration}
    , interval_{interval}
    , sequence_{s_counter_.GetAndAdd(1)}
  {
  }

  KANON_INLINE TimeStamp expiration() const KANON_NOEXCEPT { return expiration_; }
  KANON_INLINE double interval() const KANON_NOEXCEPT { return interval_; }
  KANON_INLINE bool repeat() const KANON_NOEXCEPT { return interval_ > 0.0; }
  KANON_INLINE uint64_t sequence() const KANON_NOEXCEPT { return sequence_; }
  KANON_INLINE TimerQueue *timer_queue() KANON_NOEXCEPT { return timer_queue_; }

  KANON_INLINE void BindTimerQueue(TimerQueue *queue) KANON_NOEXCEPT
  {
    timer_queue_ = queue;
  }

  KANON_INLINE void run() { callback_(); }
  KANON_INLINE void restart(TimeStamp now) KANON_NOEXCEPT
  {
    expiration_ = AddTime(now, interval_);
  }

 private:
  TimerCallback callback_;
  TimeStamp expiration_;
  double interval_;
  uint64_t sequence_;
  TimerQueue *timer_queue_ = nullptr;

  KANON_NET_NO_API static AtomicCounter64 s_counter_;
};

//!@}
} // namespace kanon

#endif // KANON_TIMER_TIMER_H
