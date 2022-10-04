#ifndef KANON_THREAD_THREADPOOL
#define KANON_THREAD_THREADPOOL

#include <queue>
#include <functional>
#include <string>

#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"
#include "kanon/util/ptr.h"

#include "kanon/thread/condition.h"
#include "kanon/thread/mutex_lock.h"

namespace kanon {

class Thread;

/**
 * \brief Bounded thread pool
 * \note
 *   Using condition variable and mutex to implemetation(i.e. producer-consumer problem)
 */
class ThreadPool : noncopyable {
public:
  typedef std::function<void()> Task;

  explicit ThreadPool(int max_queue_size = 5,
                      std::string const& name = "ThreadPool");

  /**
    * \brief join all thread to reclaim their resource
    */
  ~ThreadPool() noexcept;
  
  /**
    * \brief start the thread pool and run them
    * \param threadNum number of threads in pool
    */
  void StartRun(int thread_num);

  /**
    * \brief push task to task queue and then notify Pop() to consume it
    */
  void Push(Task task);

  void SetMaxQueueSize(int num) noexcept
  { if (num > 0) max_queue_size_ = num; }
private:
  /**
    * \brief pop the task from queue, and notify Push() to produce which can produce more task
    */
  Task Pop();

  mutable MutexLock mutex_;
  Condition not_full_ GUARDED_BY(mutex_);
  Condition not_empty_ GUARDED_BY(mutex_);

  int max_queue_size_;

  std::vector<std::unique_ptr<kanon::Thread>> threads_;
  std::queue<Task> tasks_;

  std::string name_;
};
} // namespace kanon

#endif // KANON_THREAD_THREADPOOL_H
