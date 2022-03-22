#ifndef KANON_EVENTLOOP_THREAD_H
#define KANON_EVENTLOOP_THREAD_H

#include <string>

#include "kanon/thread/thread.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/thread/condition.h"
#include "kanon/util/macro.h"

namespace kanon {

class EventLoop;

/**
 * Used for IO thread
 */
class EventLoopThread : noncopyable {
public:
  EventLoopThread(std::string const& = std::string{});
  
  // Must be called in main thread  
  EventLoop* StartRun();

  // Start loop in background thread(IO thread usually)
  void BackGroundStartLoop();

  ~EventLoopThread() noexcept;
private:
  EventLoop* loop_;
  
  MutexLock mutex_;
  Condition cond_ GUARDED_BY(mutex_);

  Thread thr_;
};

} // namespace kanon

#endif // KANON_EVENTLOOP_THREAD_H
