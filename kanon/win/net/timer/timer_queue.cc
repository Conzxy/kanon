#include "kanon/win/net/timer/timer_queue.h"

#include "kanon/net/event_loop.h"
#include "kanon/log/logger.h"

using namespace kanon;

TimerQueue::TimerQueue(EventLoop *loop)
  : Base(loop)
  , timer_queue_(::CreateTimerQueue())
{
  // FIXME Error handling
  if (!timer_queue_) {
    LOG_SYSFATAL << "Failed to call CreateTimerQueue()";
  }
}

TimerQueue::~TimerQueue() noexcept { DeleteTimerQueueEx(timer_queue_, NULL); }

VOID kanon::timer_callback(_In_ PVOID ctx, _In_ BOOLEAN timer_fired)
{
  LOG_TRACE << "Timer callback";
  if (timer_fired) {
    auto timer = (Timer *)ctx;
    timer->timer_queue()->loop()->QueueToLoop([timer]() {
      timer->run();
      if (!timer->repeat())
        timer->timer_queue()->timer_map_.erase(timer->sequence());
    });
  } else {
    LOG_WARN << "This is maybe not a valid timer";
  }
}

TimerId TimerQueue::AddTimer(TimerCallback cb, TimeStamp time, double interval)
{
  Timer timer(cb, time, interval);
  timer.BindTimerQueue(this);
  auto pair = timer_map_.emplace(timer.sequence(), std::move(timer));
  assert(pair.second);
  auto *p_timer = &pair.first->second;
  HANDLE timer_handle = INVALID_HANDLE_VALUE;
  CreateTimerQueueTimer(&timer_handle, timer_queue_, &timer_callback, p_timer,
                        time.GetMilliseconds(), interval * 1000, 0);
  return TimerId{p_timer, timer_handle};
}

void TimerQueue::CancelTimer(TimerId const id)
{
  auto timer_iter = timer_map_.find(id.timer_->sequence());
  if (timer_iter == timer_map_.end()) {
    return;
  }

  DeleteTimerQueueTimer(timer_queue_, (HANDLE)id.ctx_, NULL);
  timer_map_.erase(id.timer_->sequence());
}
