#include "kanon/thread/current_thread.h"

#include "kanon/time/time_stamp.h"

#include "kanon/util/macro.h"

#include "kanon/log/logger.h"

#include "kanon/net/poll/poller.h"
#include "kanon/net/poll/epoller.h"

#include "kanon/net/timer/timer_queue.h"

#include "kanon/net/channel.h"
#include "kanon/net/macro.h"

#include "kanon/net/event_loop.h"

#include <assert.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace kanon {

namespace detail {

/**
 * Event fd API
 */
static int CreateEventFd() noexcept {
  int evfd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

  LOG_TRACE << "eventfd: " << evfd << " created";

  if (evfd < 0) {
    LOG_SYSERROR << "eventfd() error occurred";
  }

  return evfd;
}

static void ReadEventFd(int evfd) noexcept {
  uint64_t dummy;
  if (sizeof dummy != ::read(evfd, &dummy, sizeof dummy))
    LOG_SYSERROR << "ReadEventFd() error occurred";
}

static void WriteEventFd(int evfd) noexcept {
  uint64_t dummy = 0;
  if (sizeof dummy != ::write(evfd, &dummy, sizeof dummy))
    LOG_SYSERROR << "WriteEventFd() error occurred";
}

} // namespace detail

EventLoop::EventLoop()
  : ownerThreadId_{ CurrentThread::t_tid }
  , looping_{ false }
  , quit_{ false }
  , callingFunctors_{ false }
#ifdef ENABLE_EPOLL
  , poller_{ kanon::make_unique<Epoller>(this) }
#else
  , poller_{ kanon::make_unique<Poller>(this) }
#endif
  , evfd_{ detail::CreateEventFd() }
  , ev_channel_{ kanon::make_unique<Channel>(this, evfd_) }
  , timer_queue_{ kanon::make_unique<TimerQueue>(this) }
{ 
  ev_channel_->SetReadCallback([this](TimeStamp receive_time){
    LOG_TRACE << "event receive_time: " << receive_time.ToFormattedString(true);
    this->evRead();
  });

  ev_channel_->SetWriteCallback([this](){
    this->wakeup();
  });

  ev_channel_->EnableReading();

  LOG_TRACE << "EventLoop " << this << " created";
}

EventLoop::~EventLoop()
{ 
  LOG_TRACE << "EventLoop " << this << " destroyed";
}

void EventLoop::StartLoop() {
  assert(!looping_);
  //assert(!quit_);
  this->AssertInThread();
  
  looping_ = true;
  
  LOG_TRACE << "EventLoop " << this << " loop start";

  std::vector<Channel*> activeChannels;

  while (!quit_) {
    auto receive_time = poller_->Poll(POLLTIME, activeChannels);

    for (auto& channel : activeChannels) {
      channel->HandleEvents(receive_time);
    }
  
    callFunctors();

    activeChannels.clear();
  }

  LOG_TRACE << "EventLoop " << this << " loop stop";
  
  looping_ = false;
}

void EventLoop::RunInLoop(FunctorCallback cb) {
  if (IsLoopInThread()) {
    cb();  
  } else {
    this->QueueToLoop(std::move(cb));
  }
}

void EventLoop::QueueToLoop(FunctorCallback cb) {
  {
    MutexGuard dummy(lock_);
    functors_.emplace_back(std::move(cb));
  }

  // If not in IO thread(async), may be in empty polling(It is also OK if not)
  // It is in the "call functor" phase in the (IO)loop, indicates QueueToLoop() is also
  // a functor in @var functors_, then the @var functor_ is not empty,
  // and also may be in empty polling, wakeup to avoid empty polling.
  // i.e. If in the IO thread and not calling functors, indicates
  // in the poll phase or handle events.
  if (! IsLoopInThread() ||
      callingFunctors_) {
    wakeup();
  }
}

void EventLoop::UpdateChannel(Channel* ch) {
  poller_->UpdateChannel(ch);
}

void EventLoop::RemoveChannel(Channel* ch) {
  poller_->RemoveChannel(ch);
}

void EventLoop::HasChannel(Channel* ch) {
  poller_->HasChannel(ch);
}

void EventLoop::callFunctors() {
  decltype(functors_) functors;
  {
    MutexGuard dummy{ lock_ };
    functors.swap(functors_);
  }  

  callingFunctors_ = true;
  // FIXME: auto& better?
  for (auto const& func : functors) {
    try {
      assert(func);
      func();
    } catch(std::exception const& ex) {
      LOG_ERROR << "std::exception caught in callFunctors()"
            << "what(): " << ex.what();  
      KANON_RETHROW;
      callingFunctors_ = false;
    } catch(...) {
      LOG_ERROR << "some exception caught in callFunctors()";
      KANON_RETHROW;
      callingFunctors_ = false;
    }
  }

  callingFunctors_ = false;
}

void EventLoop::AssertInThread() noexcept {
  if (!this->IsLoopInThread())
    this->abortNotInThread();
}

bool EventLoop::IsLoopInThread() noexcept {
  return CurrentThread::t_tid == ownerThreadId_;
}

void EventLoop::evRead() noexcept {
  detail::ReadEventFd(evfd_);
}

void EventLoop::wakeup() noexcept {
  detail::WriteEventFd(evfd_);
}

void EventLoop::Quit() noexcept {
  quit_ = true;

  // If in the IO thread, call wakeup() in Quit()  is not necessary,
  // because it only few cases can call Quit() successfully
  //  * timer event, but is not blocking(Can be called in this thread)
  //  * asynchronous call from other thread, will call wakeup() to avoid empty polling(might)
  if (! this->IsLoopInThread())
    this->wakeup();
}

TimerId EventLoop::RunAt(TimerCallback cb, TimeStamp expiration) {
  return timer_queue_->addTimer(std::move(cb), expiration, 0.0);
}

TimerId EventLoop::RunEvery(TimerCallback cb, TimeStamp expiration, double interval) {
  return timer_queue_->addTimer(std::move(cb), expiration, interval);
}

void EventLoop::CancelTimer(TimerId timer_id) {
  timer_queue_->CancelTimer(timer_id);
}

void EventLoop::abortNotInThread() noexcept
{ LOG_FATAL << "The Policy of \"One Loop Per Thread\" has destroied!"; }

} // namespace kanon