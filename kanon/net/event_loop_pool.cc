#include "kanon/net/event_loop_pool.h"

#include "kanon/log/logger.h"

#include "kanon/net/event_loop.h"
#include "kanon/net/event_loop_thread.h"


using namespace kanon;

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

void
EventLoopPool::StartRun() {
  base_loop_->AssertInThread();

  // If the function is called not once, warning user
  assert(!started_); 
  started_ = true;

  const size_t len = name_.size() + 32;  
  std::vector<char> buf;
  buf.reserve(len);

  for (int i = 0; i != loop_num_; ++i) {
    ::snprintf(buf.data(), len, "%s[%d]", name_.c_str(), i);
    auto loopThread = new EventLoopThread{ buf.data() };
    loop_threads_.emplace_back(std::unique_ptr<EventLoopThread>(loopThread));
    loops_.emplace_back(loopThread->StartRun());
  }

}

EventLoop*
EventLoopPool::GetNextLoop() {
  base_loop_->AssertInThread();
  assert(started_); 

  EventLoop* loop = nullptr;

  if (!loops_.empty()) {
    loop = loops_[next_];
    ++next_;
    if (next_ >= loop_num_) {
      next_ = 0;
    }

    LOG_TRACE << "Next event loop Index = " << next_;
  } else {
    loop = base_loop_;
  }

  return loop;
}

auto 
EventLoopPool::GetLoops() 
-> LoopVector* {
  base_loop_->AssertInThread();
  assert(started_);

  if (loops_.empty()) {
    return nullptr;
  } else {
    return &loops_;
  }
}
