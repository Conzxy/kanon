#ifndef KANON_THREAD_THREADPOOL
#define KANON_THREAD_THREADPOOL

#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"
#include "kanon/util/ptr.h"
#include "kanon/thread/condition.h"
#include "kanon/thread/mutex_lock.h"

#include <queue>
#include <functional>

namespace kanon {

class Thread;

/**
 * @class ThreadPool
 * @brief 
 * Bounded thread pool
 * @note
 * Using condition variable and mutex to implemetation(i.e. producer-consumer problem)
 */
class ThreadPool : noncopyable {
public:
    typedef std::function<void()> Task;

    explicit ThreadPool(
        int maxQueueSize = 5,
        std::string const& name = "ThreadPool");

    /**
     * @brief join all thread to reclaim their resource
     */
    ~ThreadPool() noexcept;
    
    /**
     * @brief start the thread pool and run them
     * @param threadNum number of threads in pool
     */
    void StartRun(int threadNum);

    /**
     * @brief push task to task queue and then notify Pop() to consume it
     */
    void Push(Task task);

    /**
     * @brief pop the task from queue, and notify Push() to produce which can produce more task
     */
    Task Pop();

    void SetMaxQueueSize(int num) noexcept
    { if (num > 0) maxQueueSize_ = num; }
private:
    /**
     * @brief callback of thread
     */
    void runOfThread();

    mutable MutexLock mutex_;
    Condition notFull_ GUARDED_BY(mutex_);
    Condition notEmpty_ GUARDED_BY(mutex_);

    int maxQueueSize_;

    std::vector<std::unique_ptr<kanon::Thread>> threads_;
    std::queue<Task> tasks_;
};
} // namespace kanon

#endif // KANON_THREAD_THREADPOOL_H