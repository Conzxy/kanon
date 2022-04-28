#include "kanon/thread/thread_pool.h"

#include "kanon/thread/thread.h"

using namespace kanon;

ThreadPool::ThreadPool(int max_queue_size,
                       std::string const& name)
  : mutex_ { }
  , not_full_{ mutex_ }
  , not_empty_{ mutex_ }
  , max_queue_size_{ max_queue_size }
  , name_{ name }
{ }

ThreadPool::~ThreadPool() noexcept {
  for (auto& thr : threads_) {
    thr->Join();
  }
}

void ThreadPool::StartRun(int thread_num) {
  MutexGuard guard{ mutex_ };

  for (int i = 0; i != thread_num; ++i) {
    auto up_thr = kanon::make_unique<Thread>([this]() {
      auto task = Pop();

      if (task) {
        // Exception is handled by Thread
        task();
      }
    }, name_ + std::to_string(i));
    
    auto p_thr = GetPointer(up_thr);
    threads_.emplace_back(std::move(up_thr));        
    p_thr->StartRun();
  }
}

void ThreadPool::Push(Task task) {
  MutexGuard guard{ mutex_ };

  while (static_cast<int>(tasks_.size()) == max_queue_size_) {
    not_full_.Wait();
  }

  tasks_.emplace(std::move(task));
  not_empty_.Notify();
}

auto ThreadPool::Pop() -> Task
{
  MutexGuard guard{ mutex_ };

  while (tasks_.size() == 0) {
    not_empty_.Wait();
  }

  auto task = std::move(tasks_.front());
  tasks_.pop();

  not_full_.Notify();
  return task;
}
