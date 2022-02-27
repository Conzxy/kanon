#ifndef KANON_EVENTLOOP_THREAD_H
#define KANON_EVENTLOOP_THREAD_H

#include "kanon/thread/thread.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/thread/condition.h"
#include "kanon/util/macro.h"

#include <string>

namespace kanon {

class EventLoop;

/**
 * Used for IO thread
 */
class EventLoopThread : noncopyable {
public:
  EventLoopThread(std::string const& = std::string{});
  
  // should be called by main thread  
  EventLoop* StartRun();

  ~EventLoopThread() noexcept;
private:
  EventLoop* loop_;
  
  MutexLock lock_;
  Condition cond_;
  Thread thr_;
};

} // namespace kanon

#endif // KANON_EVENTLOOP_THREAD_H
