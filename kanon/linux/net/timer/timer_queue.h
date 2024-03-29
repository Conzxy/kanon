#ifndef KANON_LINUX_NET_TIMER_QUEUE_H__
#define KANON_LINUX_NET_TIMER_QUEUE_H__

#include "kanon/net/timer/itimer_queue_platform.h"

#include <set>
#include <unordered_set>
#include "kanon/util/ptr.h"
#include "kanon/util/time_stamp.h"
#include "kanon/net/channel.h"

#include <third-party/xxHash/xxhash.h>

namespace kanon {

class TimerQueue : public ITimerQueuePlatform {
 public:
  using Base = ITimerQueuePlatform;

  explicit TimerQueue(EventLoop *loop);
  ~TimerQueue() KANON_NOEXCEPT override;

  virtual TimerId AddTimer(TimerCallback cb, TimeStamp time,
                           double interval) override;

  virtual void CancelTimer(TimerId const id) override;

 private:
  /*
   * Don't use std::unique_ptr as key before C++14
   * even though KeyUniquePtr is satisfied.
   *
   * KeyUniquePtr ocuppy 16 bytes(I don't like)
   */

  // using TimerPtr      = KeyUniquePtr<Timer>;

  /*
   * From the use of sequence, move the sequence to the timers, we no need to
   * maintain two mirror set(almost identical)
   */

  // using ActiveTimer   = std::pair<uint64_t,  Timer*>;
  using TimerPtr = Timer *;
  using TimerEntry = std::pair<TimerPtr, uint64_t>;

  struct TimerEntryCompare {
    // FIXME In C++14, use heterogeneous lookup
#if defined(CXX_STANDARD_14) && 0
    // Any type is also OK.
    // SFINAE only interested in type definition instead specific type
    using is_transparent = void;

    struct Helper {
      using Key = std::pair<TimeStamp, uint64_t> Key key;

      Helper(TimeEntry const &te)
        : key(te.first->expiratoin(), te.second)
      {
      }

      Helper(Key const &k)
        : key(k)
      {
      }

      // For retriving all expired timers
      Helper(TimeStamp ts)
        : key(ts, UINT64_MAX)
      {
      }
    };

    KANON_INLINE bool operator()(Helper const &x,
                                 Heler const &y) const KANON_NOEXCEPT
    {
      return x.key < y.key;
    }
#else
    KANON_INLINE bool operator()(TimerEntry const &x,
                                 TimerEntry const &y) const KANON_NOEXCEPT
    {
      auto res = (x.first->expiration().GetMicrosecondsSinceEpoch() -
                  y.first->expiration().GetMicrosecondsSinceEpoch());
      if (res == 0) {
        return x.second < y.second;
      }
      return res < 0;
    }
#endif
  };

  using TimerSet = std::set<TimerEntry, TimerEntryCompare>;
  using TimerSetValue = TimerSet::value_type;
  using TimerVector = std::vector<TimerPtr>;

  /** Helper of AddTimer() */
  bool Emplace(Timer *timer);

  /** The read callback of timerfd */
  void ProcessAllExpiredTimers(TimeStamp recv_time);

  /** Helper of ProcessAllExpiredTimers() */
  TimerVector GetExpiredTimers(TimeStamp time);
  void ResetTimers(TimerVector &, TimeStamp now);

 private:
  std::unique_ptr<Channel> timer_channel_; //!< Manage events of timerfd

  /**
   * Timer* contains timestamp, but we can't store single Timer*
   * in std::set<>. Suppose you do so, then you need to provide
   * a predicate to determine which timestamp of timer is early.
   * However, timers_ can't contains two difference timer even
   * thought them have same timestamp. Oh, std::multiset<> could
   * resolve this, but when erase timer by call CancelTimer(),
   * then all timers with same timestamp will be removed that is
   * not the right behavior we desired. Therefore, we use
   * <TimeStamp, Timer*> to identify an active timer since Timer*
   * is unique and don't represents a timestamp here.
   */
  TimerSet timers_; //!< Maintain active timers
  /* The follwing data structure used for implementing CancelTimer() */

  /**
   * To cancel a timer, we must call timers_.erase().
   * Therefore, we must return Timer* or a wrapper of it.
   * However, we also need to dereference the Timer* to get
   * timestamp to search the timer entry in timers_. There
   * is a problem, can we dereference it in any time? No!
   * we can do it when it is an active timer only. So we
   * need active_timers_ to check.
   * (Also, you could say return Timer* with its timestamp
   *  then don't need to dereference it, but for repeated
   *  timer the expired timestamp will updated when timer
   *  is expired. so, the timestamp becomes invalid)
   *
   * ActiveTimer is a pair of <uint64_t, Timer*> intead of
   * a single Timer* The uint64_t object is the timer id.
   * Because two difference timer object maybe have same
   * address, I can't use Timer* to identify timer in
   * active_timers_.
   *
   * ```cpp
   * Scenario:
   * int* p = new int(); delete p;
   * int* p2 = new int(); delete p2;
   * assert(p == p2); // Don't abort!
   * ```
   *
   * We can suppose allocator(e.g. libc's malloc) will reuse the deleted memory
   * space.
   *   - For repeated timer, it is ok, since we don't remove
   *     timer actively.
   *   - For once timer, we must remove timer, and the new
   *     timer maybe reuse the memory space of old timer.
   *     If user call CancelTimer() and TimerId is single
   *     Timer*, the canceled tiemr is not desired.
   * (Also, take the TimerId to represent ActimeTimer is
   *  more exact, but we need provide a predicate like
   *  std::pair<> to sort it, thus, just reuse std::pair<> :D)
   */
  // std::set<ActiveTimer> active_timers_; //!< Detect a timer if is active

  /**
   * Self-cancel indicates user cancel a timer in
   * the callback of the timer itself.
   * If this is a repeated timer, we still reset it.
   * To distinguish the two difference repeated timer,
   * use calling_timer_ to denote the phase of calling
   * callback of all the expired timers is not finished.
   * then callback self-cancel a timer will be detected.
   */
  bool calling_timer_; //!< Detect self-cancel

  /**
   * The self-cancel timers will be put in this container.
   * The all timers are not reset, and before the next phase of
   * calling callback will be cleared.
   */
  struct TimerEntryHash {
    KANON_INLINE uint64_t operator()(TimerEntry const &x) const KANON_NOEXCEPT
    {
      return XXH64(&x.first, sizeof x.first, time(NULL)) ^
             (XXH64(&x.second, sizeof x.second, time(NULL)) << 1);
    }
  };

  std::unordered_set<TimerEntry, TimerEntryHash>
      canceling_timers_; //!< Store the self-cancel timer to avoid reset
};
} // namespace kanon

#endif
