#ifndef KANON_THREAD_THREADPOOL
#define KANON_THREAD_THREADPOOL

#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"
#include "kanon/util/unique_ptr.h"
#include "kanon/thread/Condition.h"
#include "kanon/thread/MutexLock.h"

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
    ~ThreadPool() KANON_NOEXCEPT;
    
    /**
     * @brief start the thread pool and run them
     * @param threadNum number of threads in pool
     */
    void start(int threadNum);

    /**
     * @brief push task to task queue and then notify take() to consume it
     */
    void run(Task task);

    /**
     * @brief pop the task from queue, and notify run() to produce which can produce more task
     */
    Task take();

    /**
     * @brief callback of thread
     */
    void runOfThread();

    void setMaxQueueSize(int num) KANON_NOEXCEPT
    { if (num > 0) maxQueueSize_ = num; }
private:
    mutable MutexLock mutex_;
    Condition notFull_ GUARDED_BY(mutex_);
    Condition notEmpty_ GUARDED_BY(mutex_);

    int maxQueueSize_;

    std::vector<std::unique_ptr<kanon::Thread>> threads_;
    std::queue<Task> tasks_;
};
} // namespace kanon

#endif // KANON_THREAD_THREADPOOL_H