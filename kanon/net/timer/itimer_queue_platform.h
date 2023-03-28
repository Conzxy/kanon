#ifndef KANON_ITIMER_QUEUE_PLATFORM_H__
#define KANON_ITIMER_QUEUE_PLATFORM_H__

#include "kanon/util/noncopyable.h"
#include "kanon/net/timer/timer.h"
#include "kanon/net/timer/timer_id.h"

namespace kanon {

class EventLoop;

/**
 * \ingroup net
 * \addtogroup timer
 * \brief Manage timer and process timer events
 *
 * This is an internal module of net module
 * @{
 */
/**
 * \brief A FIFO data structure that manage timers
 *
 * User can register callback and expiration of a timer,
 * and can cancel a timer also.
 * \warning This is a internal class used for EventLoop
 */
class ITimerQueuePlatform : noncopyable {
 public:
  using TimerCallback = Timer::TimerCallback;

  /**
   * \brief Construct TimerQueue that binded to a specific loop
   * \param loop EventLoop object
   */
  explicit ITimerQueuePlatform(EventLoop *loop);
  virtual ~ITimerQueuePlatform() noexcept;

  /**
   * \brief Add a timer to TimerQueue
   * \param cb The callback of timer
   * \param time Expiration time
   * \param interval
   *   Used for repeated timer only,
   *   once timer should put 0
   * \return
   *   A TimerId object which used for cancel
   *   If this is a once timer, maybe not use it.
   */
  virtual TimerId AddTimer(TimerCallback cb, TimeStamp time,
                           double interval) = 0;

  /**
   * \brief Cancel a timer in TimerQueue
   * \param id The timer id return by AddTimer
   */
  virtual void CancelTimer(TimerId const id) = 0;

 protected:
  EventLoop *loop_; //!< Ensure one loop per thread
};

//!@}

} // namespace kanon

#endif