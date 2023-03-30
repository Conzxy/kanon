#ifndef KANON_WIN_NET_TIMER_TIMER_QUEUE_H__
#define KANON_WIN_NET_TIMER_TIMER_QUEUE_H__

#include "kanon/net/timer/itimer_queue_platform.h"

#include <unordered_set>
#include <unordered_map>
#include <windows.h>
#include <threadpoollegacyapiset.h>

namespace kanon {

KANON_NET_NO_API VOID timer_callback(_In_ PVOID ctx, _In_ BOOLEAN timer_fired);

class KANON_NET_NO_API TimerQueue : public ITimerQueuePlatform {
  friend VOID timer_callback(_In_ PVOID ctx, _In_ BOOLEAN timer_fired);

 public:
  using Base = ITimerQueuePlatform;

  explicit TimerQueue(EventLoop *loop);
  ~TimerQueue() KANON_NOEXCEPT override;

  virtual TimerId AddTimer(TimerCallback cb, TimeStamp time,
                           double interval) override;

  virtual void CancelTimer(TimerId const id) override;

  EventLoop *loop() KANON_NOEXCEPT { return loop_; }

 private:
  HANDLE timer_queue_ = INVALID_HANDLE_VALUE;
  std::unordered_map<uint64_t, Timer> timer_map_;
};

} // namespace kanon

#endif