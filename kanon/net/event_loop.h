#ifndef KANON_NET_EVENTLOOP_H
#define KANON_NET_EVENTLOOP_H

#include <vector>
#include <atomic>
#include <functional>

#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"
#include "kanon/util/ptr.h"
#include "kanon/util/raw_any.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/process/process_info.h"

#include "kanon/net/timer/timer_id.h"
#include "kanon/net/callback.h"

namespace kanon {

class TimerQueue;
class Channel;
class PollerBase;

/**
 * \ingroup net
 * \addtogroup EventLoop
 * \brief event loop facilities
 * @{
 */

/**
 * \brief A loop that process various events
 *
 * There three phases in loop:
 * (1)select/poll/epoll(event dispatching) -> (2) \n
 * (2)handle events(including timerfd) -> (3) \n
 * (3)functors -> (1) \n
 * They constrcut a loop to accept event and handle them,
 * and also handle the functors that user pushs.
 */
class EventLoop : noncopyable {
 public:
  using FunctorCallback = std::function<void()>;

  /**
   * \brief Construct default eventloop that use epoll(2) as the demultiplexer
   */
  KANON_NET_API EventLoop();

  /**
   * \brief Construct eventloop whose demultiplexer is specified by user
   * \param is_poller specify the poll(2) as the demultiplexer
   */
  KANON_NET_API explicit EventLoop(bool is_poller);

  KANON_NET_API ~EventLoop();

  //! \name Loop API
  //!@{
  /**
   * \brief Start a event loop to process events
   */
  KANON_NET_API void StartLoop();

  /**
   * \brief Quit loop
   * \note
   *   If not in thread, this will call Wakeup(),
   *   user no need to call it explicitly
   */
  KANON_NET_API void Quit() KANON_NOEXCEPT;

  /**
   * \brief Run functor in the loop
   *
   * If not in the thread, will call QueueToLoop()
   * to wait event loop can process it, otherwise, call
   * it immediately
   * \note Thread-safety
   */
  KANON_NET_API void RunInLoop(FunctorCallback);

  /**
   * \brief Queue the functor to event loop
   * \note Thread-safety
   */
  KANON_NET_API void QueueToLoop(FunctorCallback);
  //!@}

  //! \cond Channel API
  /**
   * \brief Remove channel in poller_
   * \warning This is internal API, user don't call it.
   */
  KANON_NET_NO_API void RemoveChannel(Channel *ch);

  /**
   * \brief Add or update channel in poller_
   * \warning This is internal API, user don't call it.
   */
  KANON_NET_NO_API void UpdateChannel(Channel *ch);
  //! \endcond

  /**
   * \name Edge trigger mode
   * @{
   */
  /**
   * \brief Set event loop work in edge trigger mode of epoller()
   */
  KANON_NET_API void SetEdgeTriggerMode() KANON_NOEXCEPT;

  /**
   * \brief Check event loop if work in trigger mode
   */
  KANON_NET_API bool IsEdgeTriggerMode() const KANON_NOEXCEPT;
  //!@}

  /**
   * \name Timer API
   * @{
   */
  /**
   * \brief Run callback @p cb at specific time point @p expiration
   * \return
   *   A TimerId object used for removing timer from event loop
   */
  KANON_NET_API TimerId RunAt(TimerCallback cb, TimeStamp expiration);

  /**
   * \brief Run callback @p cb after a time interval @p delay
   * \param delay in second unit
   * \return
   *   A TimerId object used for removing timer from event loop
   */
  KANON_INLINE TimerId RunAfter(TimerCallback cb, double delay)
  {
    return RunAt(std::move(cb), AddTime(TimeStamp::Now(), delay));
  }

  KANON_INLINE TimerId RunAfterMs(TimerCallback cb, uint64_t delay)
  {
    return RunAt(std::move(cb), AddTimeMs(TimeStamp::Now(), delay));
  }

  KANON_INLINE TimerId RunAfterUs(TimerCallback cb, uint64_t delay)
  {
    return RunAt(std::move(cb), AddTimeUs(TimeStamp::Now(), delay));
  }

  /**
   * \brief
   *   Run callback @p cb at a specific time pointer then
   *   run @p cb in a fixed period
   * \return
   *   A TimerId object used for removing timer from event loop
   */
  KANON_NET_API TimerId RunEvery(TimerCallback cb, TimeStamp expiration,
                                 double interval);

  /**
   * \brief
   *   Run callback @p cb in a fixed period
   * \return
   *   A TimerId object used for removing timer from event loop
   */
  KANON_INLINE TimerId RunEvery(TimerCallback cb, double interval)
  {
    return this->RunEvery(std::move(cb), AddTime(TimeStamp::Now(), interval),
                          interval);
  }

  /**
   * \brief Remove timer from event loop
   *
   * Then, the callback of timer will not be called
   */
  KANON_NET_API void CancelTimer(TimerId timer_id);
  //!@}

  //! \cond AssertLoopThread
  /**
   * \brief Check invariant for "one loop per thread" policy
   * \note Although release version, it also work
   * \warning This is a interval API, user no need to call it
   */
  KANON_INLINE void AssertInThread() KANON_NOEXCEPT
  {
    if (!IsLoopInThread()) AbortNotInThread();
  }

#if KANON___THREAD_DEFINED
  KANON_INLINE bool IsLoopInThread() KANON_NOEXCEPT
  {
    return CurrentThread::t_tid == owner_thread_id_;
  }
#else
  KANON_NET_API bool IsLoopInThread() KANON_NOEXCEPT;
#endif
  //! \endcond
  /**
   * \brief Wakeup the (e)poller to avoid sleep blocking this thread
   *
   * Write some data to kernel buffer of eventfd to avoid poll
   * block for long time
   * ((e)poll have to handle and return immediately)
   */
  KANON_NET_API void Wakeup() KANON_NOEXCEPT;

  using context_t = RawAny;

  KANON_INLINE void SetContext(context_t ctx) KANON_NOEXCEPT
  {
    context_ = std::move(ctx);
  }

  KANON_INLINE context_t &context() KANON_NOEXCEPT { return context_; }
  KANON_INLINE context_t const &context() const KANON_NOEXCEPT
  {
    return context_;
  }

 private:
  /**
   * \brief Call all functors in functors_(better name: callable)
   */
  KANON_NET_NO_API void CallFunctors();

  /**
   * \brief Read callback of eventfd
   */
  KANON_NET_NO_API void EvRead() KANON_NOEXCEPT;

  //! Abort the program if not satify the "One loop per thread" policy
  KANON_NET_NO_API void AbortNotInThread() KANON_NOEXCEPT;

 private:
  /**
   * Used for checking if in this thread
   * when calling the method of EventLoop
   */
  const process::PId owner_thread_id_; //!< Current thread Id

  /*
   * looping_ and callingFunctor_ is not exposed interface for user
   * so them are thread safe
   */

  /**
   * Used for checking invariants
   */
  bool looping_; //!< Whether this loop is working

  /**
   * User for controling quit loop
   */
  std::atomic<bool> quit_; //!< Whether this loop will be quited

  /**
   * Determining this functors is a self-register functor
   */
  bool calling_functors_; //!< Whether functors are being called

  bool is_poller_; //!< Whther demultipler(poller_) is working in poller()

  /**
   * Used for getting channels(fds) that has readied
   */
  std::unique_ptr<PollerBase> poller_; //!< Demultiplexer

  std::unique_ptr<Channel> ev_channel_; //!< Used for wakeuping

  /** Make QueueToLoop() can be called asynchronously, so need lock the
   * functors_ */
  MutexLock lock_; //!< Protect functors_
  std::vector<FunctorCallback> functors_
      GUARDED_BY(lock_); //!< Store all functors that register before phase3

  std::unique_ptr<TimerQueue> timer_queue_; //!< Used for timer API

  context_t context_;
};

//!@}
} // namespace kanon

#endif // KANON_NET_EVENTLOOP_H
