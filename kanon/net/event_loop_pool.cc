#include "kanon/net/event_loop_pool.h"

#include "kanon/log/logger.h"

#include "kanon/net/event_loop.h"
#include "kanon/net/event_loop_thread.h"


using namespace kanon;

EventLoopPool::EventLoopPool(
    EventLoop* baseLoop,
    std::string const& name)
  : baseLoop_{ baseLoop }
  , started_{ false }
  , loopNum_{ 0 }
  , next_{ 0 }
  , name_{ name }
{
}

EventLoopPool::~EventLoopPool() noexcept {
  assert(started_);
}

void
EventLoopPool::StartRun() {
  baseLoop_->AssertInThread();
  // If the function is called not once, warning user
  assert(!started_); 
  started_ = true;

  const size_t len = name_.size() + 32;  
  std::vector<char> buf;
  buf.reserve(len);

  for (int i = 0; i != loopNum_; ++i) {
    ::snprintf(buf.data(), len, "%s[%d]", name_.c_str(), i);
    auto loopThread = new EventLoopThread{ buf.data() };
    threads_.emplace_back(std::unique_ptr<EventLoopThread>(loopThread));
    loops_.emplace_back(loopThread->StartRun());
  }

}

EventLoop*
EventLoopPool::GetNextLoop() {
  baseLoop_->AssertInThread();
  assert(started_); 

  EventLoop* loop = nullptr;

  if (!loops_.empty()) {
    loop = loops_[next_];
    ++next_;
    if (next_ >= loopNum_) {
      next_ = 0;
    }

    LOG_DEBUG << "Next event loop Index = " << next_;
  } else {
    loop = baseLoop_;
  }

  return loop;
}

auto 
EventLoopPool::GetLoops() 
-> LoopVector* {
  baseLoop_->AssertInThread();
  assert(started_);

  if (loops_.empty()) {
    return nullptr;
  } else {
    return &loops_;
  }
}
