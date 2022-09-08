#include "kanon/net/event_loop_pool.h"

#include <string>

#include "kanon/log/logger.h"

#include "kanon/net/event_loop.h"
#include "kanon/net/event_loop_thread.h"

using namespace kanon;
using namespace std;

EventLoopPool::EventLoopPool(EventLoop* base_loop,
                             std::string const& name)
  : base_loop_{ base_loop }
  , started_{ false }
  , loop_num_{ 0 }
  , next_{ 0 }
  , name_{ name }
{
}

EventLoopPool::~EventLoopPool() noexcept {
  assert(started_);
}

void EventLoopPool::StartRun(ThreadInitCallback const &cb) {
  base_loop_->AssertInThread();

  // If the function is called not once, warning user
  assert(!started_); 
  started_ = true;

  const size_t len = name_.size() + 32;  
  string buf;
  buf.reserve(len);
  loop_threads_.reserve(loop_num_);

  for (int i = 0; i != loop_num_; ++i) {
    ::snprintf(&*buf.begin(), len, "%s[%d]", name_.c_str(), i);
    loop_threads_.emplace_back(new EventLoopThread(buf));
    loop_threads_[i]->StartRun(cb);
  }
}

EventLoop* EventLoopPool::GetNextLoop() {
  base_loop_->AssertInThread();
  assert(started_); 

  EventLoop* loop = nullptr;

  if (!loop_threads_.empty()) {
    loop = loop_threads_[next_]->GetLoop();
    ++next_;
    if (next_ >= loop_num_) {
      next_ = 0;
    }

    LOG_TRACE_KANON << "Next event loop Index = " << next_;
  } else {
    loop = base_loop_;
  }

  return loop;
}
