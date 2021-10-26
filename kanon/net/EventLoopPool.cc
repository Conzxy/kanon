#include "kanon/net/EventLoopPool.h"

#include "kanon/net/EventLoop.h"
#include "kanon/net/EventLoopThread.h"

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

EventLoopPool::~EventLoopPool() KANON_NOEXCEPT {
  assert(started_);
}

void
EventLoopPool::start() {
  baseLoop_->assertInThread();
  // If the function is called not once, warning user
  assert(!started_); 
  started_ = true;
  
  std::string buf;
  buf.reserve(name_.size() + 32);

  for (int i = 0; i != loopNum_; ++i) {
    ::snprintf(&buf[0], buf.capacity(), "%s[%d]", name_.c_str(), i);
    auto loopThread = new EventLoopThread{ buf };
    threads_.emplace_back(std::unique_ptr<EventLoopThread>(loopThread));
    loops_.emplace_back(loopThread->start());
  }

}

EventLoop*
EventLoopPool::getNextLoop() {
  baseLoop_->assertInThread();
  assert(statred_); 

  EventLoop* loop = nullptr;

  if (!loops_.empty()) {
    loop = loops_[next_];
    ++next_;
    if (next_ == 0)
      next_ = 0;
  } else {
    loop = baseLoop_;
  }

  return loop;
}

auto 
EventLoopPool::getLoops() 
-> LoopVector* {
  baseLoop_->assertInThread();
  assert(started_);

  if (loops_.empty()) {
    return nullptr;
  } else {
    return &loops_;
  }
}
