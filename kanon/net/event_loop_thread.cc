#include "event_loop_thread.h"
#include "kanon/net/event_loop.h"

using namespace kanon;

EventLoopThread::EventLoopThread(std::string const& name)
  : loop_{ nullptr }
  , lock_{}
  , cond_{ lock_ } 
  , thr_{ [this]() {
    // !Called in IO thread
  
    EventLoop loop{};
    {
      MutexGuard guard{ lock_ };
      loop_ = &loop;
      cond_.Notify();
    }
    
    // may quit before StartLoop() be called
    loop_->StartLoop();

    // FIXME: need mutex?
    //MutexGuard guard{ lock_ };
    loop_ = nullptr;
  }, name}
{ }

EventLoopThread::~EventLoopThread() noexcept {
  // if Quit() is not called through pointer return from StartRun()
  // we should call quit explicitly  
  if (loop_ != nullptr) {
    loop_->Quit();
    thr_.Join(); // wait loop quiting
  }
}

EventLoop*
EventLoopThread::StartRun() {
  // !Called in caller thread(instead the member thread), normally,
  // !it is very likely the main thread
  thr_.StartRun();
  
  {
    MutexGuard guard{ lock_ };
    while (loop_ == nullptr) {
      cond_.Wait();
    }  
  }
  
  return loop_;
}