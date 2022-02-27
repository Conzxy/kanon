#include "kanon/net/timer/timer_queue.h"
#include "kanon/net/event_loop.h"
#include "kanon/log/logger.h"
#include "kanon/util/macro.h"
#include "kanon/util/algo.h"

#include <unistd.h>
#include <sys/timerfd.h>

namespace kanon {

namespace detail {

static inline int CreateTimerFd() noexcept {
  auto timerfd = ::timerfd_create(CLOCK_MONOTONIC, 
                  TFD_NONBLOCK | TFD_CLOEXEC);

  LOG_TRACE << "tiemrfd: " << timerfd << " created";

  if (timerfd < 0)
    LOG_SYSERROR << "timer_create() error occurred";

  return timerfd;
}

static struct timespec getTimeFromNow(TimeStamp time) noexcept {
  int interval = time.GetMicrosecondsSinceEpoch() - TimeStamp::Now().GetMicrosecondsSinceEpoch();
  if (interval < 100) interval = 100;

  struct timespec expire;
  expire.tv_sec = static_cast<time_t>(interval / TimeStamp::kMicrosecondsPerSeconds_);
  expire.tv_nsec = static_cast<long>(interval % TimeStamp::kMicrosecondsPerSeconds_ * 1000);

  return expire;  
}

//static struct timespec getTimerInterval(double time) noexcept {
  //static constexpr int kNanoSecondPerSecond = 1000000000;

  //int64_t diff = static_cast<int64_t>(time * kNanoSecondPerSecond);
  
  //struct timespec interval;
  //interval.tv_sec = static_cast<time_t>(diff / kNanoSecondPerSecond);
  //interval.tv_nsec = static_cast<long>(diff % kNanoSecondPerSecond);

  //return interval;
//}

// static inline void print_timespec(struct timespec const& spec) noexcept {
//   LOG_DEBUG << "second: " << spec.tv_sec
//         << ";nanosecond: " << spec.tv_nsec;
// }

static inline void print_itimerspec(struct itimerspec const& spec) {
  LOG_DEBUG << "reset: expiration: " 
        << "second: " << spec.it_value.tv_sec
        << ",nanosecond: " << spec.it_value.tv_nsec << ";"
        << "interval: "
        << "second: " << spec.it_interval.tv_sec
        << ",nanosecond: " << spec.it_interval.tv_nsec;
}

static void resetTimerfd(int timerfd, Timer const& timer) noexcept {
  struct itimerspec new_value;
  BZERO(&new_value.it_interval, sizeof(struct timespec));      

  // since we use expiration time to sort timer
  // so interval is useless
  //new_value.it_interval = getTimerInterval(timer.interval());
  new_value.it_value = getTimeFromNow(timer.expiration());

  print_itimerspec(new_value);

  if (::timerfd_settime(timerfd, 0, &new_value, NULL))
    LOG_SYSERROR << "timerfd_settime error occurred";

  LOG_TRACE << "reset successfully";
}

static void readTimerfd(int timerfd) noexcept {
  uint64_t dummy = 0;
  uint64_t n;

  if ((n = ::read(timerfd, &dummy, sizeof dummy)) != sizeof dummy)
    LOG_SYSERROR << "timerfd read error";

  LOG_TRACE << "read " << n << " bytes";
}

} // namespace detail

TimerQueue::TimerQueue(EventLoop* loop)
  : timerfd_{ detail::CreateTimerFd() }
  , timer_channel_{ kanon::make_unique<Channel>(loop, timerfd_) }
  , calling_timer_{ false }
  , loop_{ loop }
{
    timer_channel_->SetReadCallback([this](TimeStamp receive_time) {
    KANON_UNUSED(receive_time);
    loop_->AssertInThread();
    
    TimeStamp now{ TimeStamp::Now() };

    detail::readTimerfd(timerfd_);  

    auto expired_timers = this->getExpiredTimers(now);
    
    // since self-cancel must be called in callback,
    // before callback, we can clear @var canceling_timers to leave some space    
    canceling_timers_.clear();
    // if calling_timer_ is true and self-cancel at same time,
    // insert it to @var canceling_timers_
    calling_timer_ = true;  
    for (auto const& timer : expired_timers) {
      timer.second->run();
    }
    calling_timer_ = false;

    this->reset(expired_timers, now);
  });

  timer_channel_->SetErrorCallback([](){
    LOG_ERROR << "timer event handle error occurred";
  });

  timer_channel_->EnableReading();
}


TimerId TimerQueue::addTimer(Timer::TimerCallback cb, 
               TimeStamp time,
               double interval) {
  auto new_timer = kanon::make_unique<Timer>(std::move(cb), time, interval);
  auto ptimer = GetPointer(new_timer);

  loop_->RunInLoop([this, ptimer, &new_timer]() {
      loop_->AssertInThread();
      
      bool earliest_update = this->emplace(std::move(new_timer));

      if (earliest_update)
        kanon::detail::resetTimerfd(timerfd_, *ptimer);
  });

  return { ptimer };
}

bool TimerQueue::emplace(std::unique_ptr<Timer> uptimer) {
  assert(timer_map_.size() == active_timer_set_.size());
  
  auto earliest_timer_iter = timer_map_.begin();
  auto ret = false;

  if (timer_map_.empty() || 
    uptimer->expiration() < earliest_timer_iter->first) {
    ret = true;
  }

  {
    active_timer_set_.insert(
        std::make_pair(
          GetPointer(uptimer),
          uptimer->sequence()));
  }
  {
    timer_map_.insert(
        std::make_pair(
          uptimer->expiration(),
          std::move(uptimer)));
  }

  LOG_TRACE << "now timer total num: " << timer_map_.size();  
  return ret;

}

auto TimerQueue::getExpiredTimers(TimeStamp time) 
  -> TimerVector {
  assert(!timer_map_.empty());
  
  TimerVector expireds;

  auto expired_end = timer_map_.lower_bound(time);
  
  // LOG_DEBUG << (expired_end != timer_map_.end() ? expired_end->second->expiration().ToFormattedString(true) : "");
  // LOG_DEBUG << "expire time: " << time.ToFormattedString(true);  
  // LOG_DEBUG << "expired timer: " << std::distance(timer_map_.begin(), expired_end);

  assert(expired_end == timer_map_.end() || time < expired_end->first);
  
  std::copy(std::make_move_iterator(timer_map_.begin()), 
        std::make_move_iterator(expired_end), 
        std::back_inserter(expireds));
  
  // Must erase all active_timer in @var active_timer_set_,
  // otherwise we can't use @var active_timer_set_ to tell the timer whether live or not.
  //
  // At first, I don't erase active_timer in set,
  // therefore, if active_timer is erased actually which indicate it is canceled by user although self-cancel.
  // But when implemating CancelTimer(), we also need consider how to erase timer in map,
  // you can't erase timer in map easily now.
  //
  // Right approach is set a canceling_timers_,
  // if timer in canceling_timer_, we don't reset repeat timer.
  //
  for (auto& expired_timer : expireds) {
    //if (!expired_timer.second->repeat())
      timer_map_.erase(expired_timer.first);
    active_timer_set_.erase(std::make_pair(
          GetPointer(expired_timer.second),
          expired_timer.second->sequence()));
  }  
    
  return expireds;    
}

void 
TimerQueue::reset(TimerVector& expireds, TimeStamp now) {
  Timer* next_expire{ nullptr };
  
  for (auto& timer : expireds) {
    // if timer is repeat, we should restart it
    // but if it in such case as self-cancel,
    // we shouldn't restart it and do nothing.
    auto ptimer = GetPointer(timer.second);
    if (timer.second->repeat()) {
      if (!contains(canceling_timers_, 
          ActiveTimer{ ptimer, ptimer->sequence() })) {
        // auto ptimer = std::move(timer.second);
        // auto tmp = GetPointer(ptimer);
        
        // timer_map_.erase(timer.first);
        // ptimer->restart(now);
        
        // timer_map_.emplace(std::make_pair(
        //       ptimer->expiration(),
        //       std::move(ptimer)));
      
        // active_timer_set_.emplace(ActiveTimer{ tmp, tmp->sequence() });

        // if (timer_map_.begin()->first > tmp->expiration())
        //   detail::resetTimerfd(timerfd_, *tmp);
        timer.second->restart(now);
        this->emplace(std::move(timer.second));
      }
    }
  }
  
  // LOG_DEBUG << "reset timer_map size: " << timer_map_.size();

  if (!timer_map_.empty()) {
    next_expire = timer_map_.begin()->second.get();
  }

  if (next_expire) {
    detail::resetTimerfd(timerfd_, *next_expire);
  }
}

void
TimerQueue::CancelTimer(TimerId const& id) {
  // LOG_DEBUG << "cancelTimer: "
  //   << id.timer_->expiration().ToFormattedString(true) << id.seq_;

  loop_->RunInLoop([this, &id]() {
    loop_->AssertInThread();
    
    auto active_timer = active_timer_set_.find(ActiveTimer{ id.timer_, id.seq_ });

    if (active_timer_set_.end() != active_timer) {
      active_timer_set_.erase(active_timer);
      timer_map_.erase(active_timer->first->expiration());
    } else if (calling_timer_) {
      // this case is self-cancel
      canceling_timers_.insert(ActiveTimer { id.timer_, id.seq_ });
    }
    
  });

}

} // namespace kanon
