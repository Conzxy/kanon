#ifndef KANON_EVENTLOOP_THREAD_H
#define KANON_EVENTLOOP_THREAD_H

#include <string>

#include "kanon/thread/condition.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/thread/thread.h"
#include "kanon/util/macro.h"

namespace kanon {

class EventLoop;

//! \addtogroup EventLoop
//!@{

/**
 * \brief Represents a event loop but running in another thread
 * \note
 *   This is used for IO thread
 */
class EventLoopThread : noncopyable {
 public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  KANON_NET_API EventLoopThread(std::string const & = std::string{});
  KANON_NET_API ~EventLoopThread() KANON_NOEXCEPT;

  /**
   * \brief Start a new thread and start event loop
   * \note
   *   Must be called in main thread
   */
  KANON_INLINE EventLoop *StartRun() { return StartRun({}); }

  KANON_NET_API EventLoop *StartRun(ThreadInitCallback const &init_cb);
  KANON_NET_API EventLoop *GetLoop() KANON_NOEXCEPT { return loop_; }
  KANON_NET_API EventLoop const *GetLoop() const KANON_NOEXCEPT
  {
    return loop_;
  }

 private:
  // Start loop in background thread(IO thread usually)
  KANON_NET_NO_API void BackGroundStartLoop(ThreadInitCallback const &init_cb);

  EventLoop *loop_;

  MutexLock mutex_;
  Condition cond_ GUARDED_BY(mutex_);

  Thread thr_;
};

//!@}
} // namespace kanon

#endif // KANON_EVENTLOOP_THREAD_H
