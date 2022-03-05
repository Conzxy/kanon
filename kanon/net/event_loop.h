#ifndef KANON_NET_EVENTLOOP_H
#define KANON_NET_EVENTLOOP_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/ptr.h"
#include "kanon/util/macro.h"

#include "kanon/thread/mutex_lock.h"

#include "timer/timer_id.h"

#include "callback.h"

#include <vector>
#include <sys/types.h>
#include <atomic>
#include <functional>

namespace kanon {

class TimerQueue;
class Channel;
class PollerBase;

#ifdef NDEBUG
#define POLLTIME 10000 // 10 seconds
#else
#define POLLTIME 1000 // 1 seconds
#endif 

/**
 * There three phases in loop:
 * (1)select/poll/epoll(event dispatching) -> (2) 
 * (2)handle events(including timerfd) -> (3)
 * (3)functors -> (1)
 * They constrcut a loop to accept event and handle them, 
 * and also handle the functors that user pushs.
 */
class EventLoop : noncopyable {
public:
  using FunctorCallback = std::function<void()>;

  EventLoop();
  ~EventLoop();
  
  /**
   * Start a event loop
   */
  void StartLoop();

  /**
   * Quit loop
   * @note if not in thread, should call Wakeup()
   */
  void Quit() noexcept;


  /**
   * Run functor in the loop
   * * If not in the thread, should append to @var functors_
   * * Otherwise, call it immediately
   * @note Threas-safety
   */
  void RunInLoop(FunctorCallback);

  /**
   * queue the functor
   * @note Thread-safety 
   */
  void QueueToLoop(FunctorCallback);
  
  /**
   * XXXChannel is just forward to poll to do something
   * @warning This is internal API, user don't call it.
   */
  void RemoveChannel(Channel* ch);
  void UpdateChannel(Channel* ch);
  void HasChannel(Channel* ch);

  /**
   * @brief timer API
   */
  TimerId RunAt(TimerCallback cb, TimeStamp expiration);

  TimerId RunAfter(TimerCallback cb, double delay) {
    return this->RunAt(std::move(cb), AddTime(TimeStamp::Now(), delay));
  }

  TimerId RunEvery(TimerCallback cb, TimeStamp expiration, double interval);

  TimerId RunEvery(TimerCallback cb, double interval) {
    return this->RunEvery(std::move(cb), AddTime(TimeStamp::Now(), interval), interval);
  }

  void CancelTimer(TimerId timer_id);

  //void CancelTimer(TimerId id);

  /**
   * @brief maintain invariant for "one loop per thread" policy
   * @note although release version, it also work
   */
  void AssertInThread() noexcept;
  bool IsLoopInThread() noexcept;
private:
  /**
   * @brief write some data to kernel buffer of eventfd,
   *      to avoid poll block for long time
   *      ((e)poll have to handle and return at once)
   */
  void Wakeup() noexcept;

  /**
   * Call all functors in functors_(better name: callable)
   */
  void CallFunctors();

  /**
   * @brief read callback of eventfd
   */
  void EvRead() noexcept;
  
  void AbortNotInThread() noexcept;
private:
  /** 
   * Used for checking if in this thread 
   * when calling the method of EventLoop
   */
  const pid_t ownerThreadId_;

  // looping_ and callingFunctor_ is not exposed interface for user
  // so them are thread safe
  bool looping_;
  std::atomic<bool> quit_;
  bool callingFunctors_;

  /** 
   * Used for getting channels(fds) that has readied 
   */
  std::unique_ptr<PollerBase> poller_;
  //std::vector<Channel*> activeChannels_;

  /** Used for wakeuping */
  int evfd_;
  std::unique_ptr<Channel> ev_channel_;

  /** QueueToLoop() can be called asynchronously, need lock the @var functors_ */
  MutexLock lock_;
  std::vector<FunctorCallback> functors_ GUARDED_BY(lock_);

  /** Used for timer API */
  std::unique_ptr<TimerQueue> timer_queue_;
};

}


#endif // KANON_NET_EVENTLOOP_H
