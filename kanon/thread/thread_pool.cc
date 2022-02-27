#include "kanon/thread/thread_pool.h"

#include "kanon/thread/thread.h"

using namespace kanon;

ThreadPool::ThreadPool(
  int maxQueueSize,
  std::string const& name)
  : mutex_ { }
  , notFull_{ mutex_ }
  , notEmpty_{ mutex_ }
  , maxQueueSize_{ maxQueueSize }
{ }

ThreadPool::~ThreadPool() noexcept {
  for (auto& thr : threads_) {
    thr->Join();
  }
}

void
ThreadPool::StartRun(int threadNum) {
  MutexGuard guard{ mutex_ };

  for (int i = 0; i != threadNum; ++i) {
    auto up_thr = kanon::make_unique<Thread>([this]() {
      this->runOfThread();
    });
    
    auto p_thr = GetPointer(up_thr);
    threads_.emplace_back(std::move(up_thr));        
    p_thr->StartRun();
  }
}

void
ThreadPool::Push(Task task) {
  MutexGuard guard{ mutex_ };

  if (static_cast<int>(tasks_.size()) == maxQueueSize_) {
    notFull_.Wait();
  }

  tasks_.emplace(std::move(task));
  notEmpty_.Notify();
}

auto
ThreadPool::Pop()
  -> Task
{
  MutexGuard guard{ mutex_ };

  if (tasks_.size() == 0) {
    notEmpty_.Wait();
  }

  auto task = std::move(tasks_.front());
  tasks_.pop();

  notFull_.Notify();
  return task;
}

void
ThreadPool::runOfThread() {
  auto task = Pop();

  if (task) {
    task();
  }
}
