#include "kanon/net/event_loop_thread.h"

#include "kanon/net/event_loop.h"

using namespace kanon;

EventLoopThread::EventLoopThread(std::string const& name)
  : loop_{ nullptr }
  , mutex_{ }
  , cond_{ mutex_ } 
  , thr_{ name }
{ 
}

EventLoopThread::~EventLoopThread() noexcept {
  // if Quit() is not called through pointer return from StartRun()
  // we should call quit explicitly  
  if (loop_ != nullptr) {
    loop_->Quit();
    thr_.Join(); // wait loop quiting
  }
}

EventLoop* EventLoopThread::StartRun() {
  thr_.StartRun(std::bind(&EventLoopThread::BackGroundStartLoop, this));
  
  {
    MutexGuard guard{ mutex_ };
    while (loop_ == nullptr) {
      cond_.Wait();
    }  
  }
  
  return loop_;
}

void EventLoopThread::BackGroundStartLoop()
{    
    // Called in IO thread
    EventLoop loop;
    {
      MutexGuard guard{ mutex_ };
      loop_ = &loop;
      cond_.Notify();
    }
    
    // Maybe quit before StartLoop() be called,
    // but it is harmless.
    loop_->StartLoop();

    loop_ = nullptr;
}