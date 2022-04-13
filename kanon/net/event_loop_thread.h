#ifndef KANON_EVENTLOOP_THREAD_H
#define KANON_EVENTLOOP_THREAD_H

#include <string>

#include "kanon/thread/thread.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/thread/condition.h"
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
  EventLoopThread(std::string const& = std::string{});
  ~EventLoopThread() noexcept;

  /**
   * \brief Start a new thread and start event loop
   * \note
   *   Must be called in main thread  
   */
  EventLoop* StartRun();

private:
  // Start loop in background thread(IO thread usually)
  void BackGroundStartLoop();

  EventLoop* loop_;
  
  MutexLock mutex_;
  Condition cond_ GUARDED_BY(mutex_);

  Thread thr_;
};

//!@}
} // namespace kanon

#endif // KANON_EVENTLOOP_THREAD_H
