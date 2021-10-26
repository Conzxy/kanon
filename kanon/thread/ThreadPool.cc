#include "kanon/thread/ThreadPool.h"

#include "kanon/thread/Thread.h"

using namespace kanon;

ThreadPool::ThreadPool(
        int maxQueueSize,
        std::string const& name)
    : mutex_ { }
    , notFull_{ mutex_ }
    , notEmpty_{ mutex_ }
    , maxQueueSize_{ maxQueueSize }
{ }

ThreadPool::~ThreadPool() KANON_NOEXCEPT {
    for (auto& thr : threads_) {
        thr->join();
    }
}

void
ThreadPool::start(int threadNum) {
    MutexGuard guard{ mutex_ };

    for (int i = 0; i != threadNum; ++i) {
        auto up_thr = kanon::make_unique<Thread>([this]() {
                this->runOfThread();
            });

        threads_.emplace_back(std::move(up_thr));        
        threads_[i]->start();
    }
}

void
ThreadPool::run(Task task) {
    MutexGuard guard{ mutex_ };

    if (static_cast<int>(tasks_.size()) == maxQueueSize_) {
        notFull_.wait();
    }

    tasks_.emplace(std::move(task));
    notEmpty_.notify();
}

auto
ThreadPool::take()
  -> Task
{
    MutexGuard guard{ mutex_ };

    if (tasks_.size() == 0) {
        notEmpty_.wait();
    }

    auto task = std::move(tasks_.front());
    tasks_.pop();

    notFull_.notify();
    return task;
}

void
ThreadPool::runOfThread() {
    auto task = take();

    if (task) {
        task();
    }
}