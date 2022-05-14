#include "kanon/net/timer/timer_queue.h"

#include <unistd.h>
#include <sys/timerfd.h>

#include "kanon/net/callback.h"
#include "kanon/net/event_loop.h"
#include "kanon/log/logger.h"
#include "kanon/net/timer/timer.h"
#include "kanon/util/macro.h"
#include "kanon/util/algo.h"
#include "kanon/util/mem.h"
#include "kanon/util/ptr.h"

namespace kanon {

// Some wrapper API about timerfd, such as set and reset...
namespace detail {

static constexpr int64_t kNanosecond = 1000000000;

static inline int CreateTimerFd() noexcept {
  auto timerfd = ::timerfd_create(
                  CLOCK_MONOTONIC,
                  TFD_NONBLOCK | TFD_CLOEXEC);

  LOG_TRACE_KANON << "Timer Fd: " << timerfd << " created";

  if (timerfd < 0) {
    LOG_SYSERROR << "::timer_create() error occurred";
  }

  return timerfd;
}

static inline struct timespec GetTimeFromNow(TimeStamp time) noexcept {
  int64_t interval = time.GetMicrosecondsSinceEpoch() - TimeStamp::Now().GetMicrosecondsSinceEpoch();
  if (interval < 100) interval = 100;

  struct timespec expire;
  expire.tv_sec = static_cast<time_t>(interval / TimeStamp::kMicrosecondsPerSeconds_);
  expire.tv_nsec = static_cast<long>(interval % TimeStamp::kMicrosecondsPerSeconds_ * 1000);

  return expire;  
}

static inline struct timespec GetTimerInterval(double interval) noexcept {
  const int64_t val = interval * kNanosecond;
  struct timespec spec;

  spec.tv_sec = static_cast<time_t>(val / kNanosecond);
  spec.tv_nsec = static_cast<long>(val % kNanosecond);

  return spec;
}

static inline void PrintItimerspec(struct itimerspec const& spec) {
#ifndef NDEBUG
  LOG_DEBUG_KANON << "Reset the expiration(sec, nsec): (" 
            << spec.it_value.tv_sec << ", "
            << spec.it_value.tv_nsec << ")";
#endif
}

static inline void SetTimerFd(int timerfd, Timer const& timer, bool set_interval = false) noexcept {
  struct itimerspec new_value;
  MemoryZero(new_value.it_interval);
  // since we use expiration time to sort timer
  // so interval is useless

  if (set_interval && timer.repeat()) {
    new_value.it_interval = GetTimerInterval(timer.interval());
  }

  new_value.it_value = GetTimeFromNow(timer.expiration());

  PrintItimerspec(new_value);

  // The default interpretation of .it_value is relative time
  if (::timerfd_settime(timerfd, 0, &new_value, NULL)) {
    LOG_SYSERROR << "::timerfd_settime() error occurred";
  }
  else {
    LOG_TRACE_KANON << "Reset successfully";
  }
}

static inline void ReadTimerFd(int timerfd) noexcept {
  uint64_t dummy = 0;
  uint64_t n = 0;

  if ((n = ::read(timerfd, &dummy, sizeof dummy)) != sizeof dummy) {
    LOG_SYSERROR << "::read() of timerfd error occurred";
  }
  else {
    LOG_TRACE_KANON << "Read " << n << " bytes";
  }
}

} // namespace detail

TimerQueue::TimerQueue(EventLoop* loop)
  : timer_channel_{ kanon::make_unique<Channel>(loop, detail::CreateTimerFd()) }
  , calling_timer_{ false }
  , loop_{ loop }
{
  timer_channel_->SetReadCallback(std::bind(
    &TimerQueue::ProcessAllExpiredTimers, this, _1));

  timer_channel_->SetErrorCallback([](){
    LOG_SYSERROR << "Timer event handler error occurred";
  });

  timer_channel_->EnableReading();
}


TimerId TimerQueue::AddTimer(
  Timer::TimerCallback cb, 
  TimeStamp time,
  double interval) 
{
  Timer* timer = new Timer(std::move(cb), time, interval);
  loop_->RunInLoop([this, timer]() {
    loop_->AssertInThread();
    
    bool earliest_update = Emplace(timer);

    if (earliest_update) {
      kanon::detail::SetTimerFd(timer_channel_->GetFd(), *timer);
    }
  });

  return timer;
}

void TimerQueue::CancelTimer(TimerId id) {
  LOG_DEBUG_KANON << "CancelTimer: "
    << "TimerId = " << id.seq_;

  loop_->RunInLoop([this, id]() {
    loop_->AssertInThread();
    
    const auto active_timer = active_timers_.find(ActiveTimer(
      id.seq_, id.timer_));
    if (active_timers_.end() != active_timer) {
      active_timers_.erase(active_timer);
      timers_.erase(TimerEntry(
        id.timer_->expiration(), MakeUniquePtrAsKey(id.timer_)));
    } 
    else if (calling_timer_) {
      // Self-cancel
      // \see ProcessAllExpiredTimers()
      canceling_timers_.emplace(id.seq_, id.timer_);
    }
  });
}

bool TimerQueue::Emplace(Timer* timer) {
  assert(timers_.size() == active_timers_.size());
  
  auto earliest_timer_iter = timers_.begin();
  auto ret = false;

  if (timers_.empty() || 
    timer->expiration() < earliest_timer_iter->first) {
    ret = true;
  }

  active_timers_.emplace(timer->sequence(), timer);
  timers_.emplace(timer->expiration(), TimerPtr(timer));

  LOG_TRACE_KANON << "Now total timer count = " << timers_.size();  
  return ret;
}

void TimerQueue::ProcessAllExpiredTimers(TimeStamp recv_time) {
  KANON_UNUSED(recv_time);
  loop_->AssertInThread();

  // Read the message from timer to avoid busy loop 
  // since level trigger
  detail::ReadTimerFd(timer_channel_->GetFd());  

  // Get expired time from kernel and put it to GetExpiredTimers()
  TimeStamp now{ TimeStamp::Now() };
  auto expired_timers = GetExpiredTimers(now);
  
  // Because the canceling_timers_ only useful in the ResetTimers(),
  // we can clear all the self-cancel timers since them are useless
  canceling_timers_.clear();
  // Self-cancel must be called in the callback of timer,
  // then callback will emplace the canceling timer to 
  // canceling_timers_ according the calling_timer_ is true.
  // Then in ResetTimers() can check a timer if is a self-cancel
  // timer, and if it is, don't reset it althought it should be 
  // reset.
  calling_timer_ = true;

  for (auto const& timer : expired_timers) {
    try {
      timer->run();
    }
    catch (std::exception const& ex) {
      LOG_ERROR << "caught the std::exception in calling of timer callback";
      LOG_ERROR << "exception message: " << ex.what();
      KANON_RETHROW;
    }
    catch (...) {
      LOG_ERROR << "caught the unknown exception in calling of timer callback";
      KANON_RETHROW;
    }
  }

  calling_timer_ = false;
  ResetTimers(expired_timers, now);
}

auto TimerQueue::GetExpiredTimers(TimeStamp time) -> TimerVector {
  TimerVector expireds;

  // Get the first timer which does not expired
  // i.e. the after position of the last timer which has expired

  auto expired_end = timers_.lower_bound(TimerEntry(
    time, MakeUniquePtrAsKey<Timer>(
      reinterpret_cast<Timer*>(UINTPTR_MAX) ) ) );

  // [begin(), expired_end) are all timers which has expired 
  std::transform(
    timers_.begin(),
    expired_end,
    std::back_inserter(expireds),
    [](TimerSetValue const& x) -> TimerPtr {
      // The return value of TimerSet::iterator is const key_type
      // So, argument type must be const& and need const_cast<>
      return TimerPtr(const_cast<TimerPtr&>(x.second).release());
      // return std::move(const_cast<TimerSetValue&>(x).second);
    });

  timers_.erase(timers_.begin(), expired_end);
  LOG_DEBUG_KANON << "Expired time =  " << time.ToFormattedString(true);  
  LOG_DEBUG_KANON << "Expired timer count = " << expireds.size();
  
  for (auto& expired_timer : expireds) {
    //if (!expired_timer.second->repeat())
    active_timers_.erase(ActiveTimer(
      expired_timer->sequence(), GetPointer(expired_timer)));
  }  
    
  return expireds;
}

void TimerQueue::ResetTimers(TimerVector& expireds, TimeStamp now) {
  Timer* next_expire{ nullptr };
  
  for (auto& timer : expireds) {
    if (timer->repeat()) {
      // If timer is repeat, we should restart it
      // but if it in such case as self-cancel,
      // we shouldn't restart it and do nothing.
      if (canceling_timers_.find(ActiveTimer(
        timer->sequence(), GetPointer(timer)) ) 
          == canceling_timers_.end()) {
        timer->restart(now);
        Emplace(timer.release());
      }
    }
  }
  
  LOG_DEBUG_KANON << "Reset timer_map size: " << timers_.size();

  if (!timers_.empty()) {
    next_expire = timers_.begin()->second.get();
  }

  if (next_expire) {
    detail::SetTimerFd(timer_channel_->GetFd(), *next_expire);
  }
}


} // namespace kanon
