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
class Timer : noncopyable {
 public:
  // friend class TimerQueue;
  typedef std::function<void()> TimerCallback;

  Timer(TimerCallback cb, TimeStamp expiration, double interval)
    : callback_{std::move(cb)}
    , expiration_{expiration}
    , interval_{interval}
    , sequence_{s_counter_.GetAndAdd(1)}
  {
  }

  TimeStamp expiration() const noexcept { return expiration_; }
  double interval() const noexcept { return interval_; }
  bool repeat() const noexcept { return interval_ > 0.0; }
  uint64_t sequence() const noexcept { return sequence_; }
  TimerQueue *timer_queue() noexcept { return timer_queue_; }

  void BindTimerQueue(TimerQueue *queue) noexcept { timer_queue_ = queue; }

  void run() { callback_(); }
  void restart(TimeStamp now) noexcept
  {
    expiration_ = AddTime(now, interval_);
  }

 private:
  TimerCallback callback_;
  TimeStamp expiration_;
  double interval_;
  uint64_t sequence_;
  TimerQueue *timer_queue_ = nullptr;

  static AtomicCounter64 s_counter_;
};

//!@}
} // namespace kanon

#endif // KANON_TIMER_TIMER_H
