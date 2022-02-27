#ifndef KANON_TIMER_TIMER_H
#define KANON_TIMER_TIMER_H

#include "kanon/time/time_stamp.h"
#include "kanon/thread/atomic.h"
#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"

#include <functional>

namespace kanon {

/**
 * @brief encapsulation of itimespec
 */
class Timer : noncopyable {
public:
    typedef std::function<void()> TimerCallback;

    Timer(TimerCallback cb, TimeStamp expiration, double interval)
        : callback_{ std::move(cb) }
        , expiration_{ expiration }
        , interval_{ interval }
        , repeat_{ interval > 0.0 }
        , sequence_{ s_counter_.incrementAndGet() }
    { }

    void run() {
        callback_();
    }


    TimeStamp expiration() const noexcept { return expiration_; }
    double interval() const noexcept { return interval_; }
    bool repeat() const noexcept { return repeat_; }
    int sequence() const noexcept { return sequence_; }
  void restart(TimeStamp now) noexcept;
private:
    TimerCallback callback_;
    TimeStamp expiration_; 
    double interval_;
    bool repeat_; 
    int64_t sequence_;

    static AtomicInt64 s_counter_;

};

} // namespace kanon

#endif // KANON_TIMER_TIMER_H
