#include "kanon/net/event_loop.h"

#include <assert.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "kanon/thread/current_thread.h"
#include "kanon/util/ptr.h"
#include "kanon/util/time_stamp.h"
#include "kanon/util/macro.h"
#include "kanon/log/logger.h"

#include "kanon/net/poll/poller.h"
#include "kanon/net/poll/epoller.h"
#include "kanon/net/timer/timer_queue.h"
#include "kanon/net/channel.h"
#include "kanon/net/macro.h"

namespace kanon {

namespace detail {

/**
 * Event fd API
 */
static inline int CreateEventFd() noexcept {
  int evfd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

  LOG_TRACE << "eventfd: " << evfd << " created";

  if (evfd < 0) {
    LOG_SYSERROR << "eventfd() error occurred";
  }

  return evfd;
}

/**
 * Eventfd maintains a counter.
 * The write() adds the 8-byte integer value to counter.
 * The read() will block if counter is zero.
 * Therefore, the dummy of write is must not be zero.
 * \see man eventfd(2)
 */
static inline void ReadEventFd(int evfd) noexcept {
  uint64_t dummy;
  if (sizeof dummy != ::read(evfd, &dummy, sizeof dummy))
    LOG_SYSERROR << "ReadEventFd() error occurred";
}

static inline void WriteEventFd(int evfd) noexcept {
  uint64_t dummy = 1;
  if (sizeof dummy != ::write(evfd, &dummy, sizeof dummy))
    LOG_SYSERROR << "WriteEventFd() error occurred";
}

} // namespace detail

EventLoop::EventLoop()
  : EventLoop(false)
{

}

EventLoop::EventLoop(bool is_poller)
  : owner_thread_id_{ CurrentThread::t_tid }
  , looping_{ false }
  , quit_{ false }
  , calling_functors_{ false }
  , is_poller_{ is_poller }
#ifdef ENABLE_EPOLL
  , poller_{ is_poller_ ?
     static_cast<PollerBase*>(new Poller(this)) :
     static_cast<PollerBase*>(new Epoller(this)) }
#else
  , poller_{ std::make_unique<Poller>(this) }
#endif
  // CreateEventfd() is don't throw exception, don't wrong exception-safety
  , ev_channel_{ kanon::make_unique<Channel>(this, detail::CreateEventFd()) }
  , timer_queue_{ kanon::make_unique<TimerQueue>(this) }
{ 

  ev_channel_->SetReadCallback([this](TimeStamp receive_time){
    LOG_TRACE << "EventFd receive_time: " << receive_time.ToFormattedString(true);
    this->EvRead();
  });

  ev_channel_->SetWriteCallback([this](){
    this->Wakeup();
  });

  ev_channel_->EnableReading();

  LOG_TRACE << "EventLoop " << this << " created";
}

EventLoop::~EventLoop()
{ 
  // No need to Quit() explicitly here:
  // 1. If in loop, call it by user instead of library
  // 2. If not in loop, call it by EventLoopThread so
  //    it don't considered by user.
  LOG_TRACE << "EventLoop " << this << " destroyed";

  assert(!looping_);
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
  
    CallFunctors();

    activeChannels.clear();
  }

  LOG_TRACE << "EventLoop " << this << " loop stop";
  
  looping_ = false;
}

void EventLoop::RunInLoop(FunctorCallback cb) {
  if (IsLoopInThread()) {
    try {
      cb();
    } catch(std::exception const& ex) {
      LOG_ERROR << "std::exception(including its derived class) is caught in RunInLoop()";
      LOG_ERROR << "Reason: " << ex.what();
      KANON_RETHROW;
    } catch(...) {
      LOG_ERROR << "Unknown exception is caught in RunInLoop()";
      KANON_RETHROW;
    }
  } else {
    this->QueueToLoop(std::move(cb));
  }
}

void EventLoop::QueueToLoop(FunctorCallback cb) {
  {
    MutexGuard dummy(lock_);
    functors_.emplace_back(std::move(cb));
  }

  // If not in IO thread(async), and not event occurred, then block.
  // That's wrong, since it is expected to be called immediately.
  // e.g. 
  // async call the Connect() of TcpClient(or derived class)
  // there is no event occurred, but we should call it intead
  // of block POLL_TIME(e.g. 10s).

  // If call QueueToLoop() in the "call functor" phase(i.e. phase 3), we can't ensure
  // the next loop don't do a empty poll(and block), should Wakeup() eventfd to avoid
  // it. The other two phase it OK, since phase 3 in after.
  // e.g. 
  // Sending file in pipeline way.
  // If the first write is not complete, we need to write it again.
  // Fortunatly, kanon provide WriteCompleteCallback can implemete
  // it. We first register it in phase2(Suppose we send it in ConnectionCallback)
  // then WriteCompleteCallback continue write and register it if 
  // not complete, but this is in the phase3, if we don't Wakeup(),
  // the next loop must be blocked.
  // \see example/file_transfer/client.cc
  if (!IsLoopInThread() || calling_functors_) {
    Wakeup();
  }
}

void EventLoop::UpdateChannel(Channel* ch) {
  poller_->UpdateChannel(ch);
}

void EventLoop::RemoveChannel(Channel* ch) {
  poller_->RemoveChannel(ch);
}

void EventLoop::CallFunctors() {
  decltype(functors_) functors;
  {
    MutexGuard dummy{ lock_ };
    functors.swap(functors_);
  }  

  calling_functors_ = true;
  for (auto& func : functors) {
    try {
      if (func) {
        func();
      }
    } catch(std::exception const& ex) {
      LOG_ERROR << "std::exception caught in CallFunctors()";
      LOG_ERROR << "Reason: " << ex.what();  
      calling_functors_ = false;
      KANON_RETHROW;
    } catch(...) {
      LOG_ERROR << "Unknown exception caught in CallFunctors()";
      calling_functors_ = false;
      KANON_RETHROW;
    }
  }

  calling_functors_ = false;
}

void EventLoop::AssertInThread() noexcept {
  if (!this->IsLoopInThread())
    this->AbortNotInThread();
}

bool EventLoop::IsLoopInThread() noexcept {
  return CurrentThread::t_tid == owner_thread_id_;
}

void EventLoop::EvRead() noexcept {
  detail::ReadEventFd(ev_channel_->GetFd());
}

void EventLoop::Wakeup() noexcept {
  detail::WriteEventFd(ev_channel_->GetFd());
}

void EventLoop::Quit() noexcept {
  quit_ = true;

  LOG_DEBUG << "EventLoop is quiting";

  // If in the IO thread, call Wakeup() in Quit() is not necessary,
  // because it only few cases can call Quit() successfully
  // * In the loop, the only choice is timer event, but it is not blocking
  // * In the other thread, do an asynchronous call, need call Wakeup() to 
  //   avoid empty poll like QueueToLoop()
  if (! this->IsLoopInThread())
    this->Wakeup();
}

TimerId EventLoop::RunAt(TimerCallback cb, TimeStamp expiration) {
  return timer_queue_->AddTimer(std::move(cb), expiration, 0.0);
}

TimerId EventLoop::RunEvery(TimerCallback cb, TimeStamp expiration, double interval) {
  return timer_queue_->AddTimer(std::move(cb), expiration, interval);
}

void EventLoop::CancelTimer(TimerId timer_id) {
  timer_queue_->CancelTimer(timer_id);
}

void EventLoop::AbortNotInThread() noexcept
{ LOG_FATAL << "The policy of \"One Loop Per Thread\" has destroyed!"; }

void EventLoop::SetEdgeTriggerMode() noexcept
{
#ifdef ENABLE_EPOLL
  if (!is_poller_) {
    auto ptr = kanon::down_pointer_cast<Epoller>(poller_.get());
    KANON_ASSERT(ptr, "This must be a Epoller*");
    ptr->SetEdgeTriggertMode();
    LOG_TRACE << "The Poller will working in edge-trigger mode";
  }
  else {
    LOG_TRACE << "poll(2) can't set to edge-trigger mode, the only mode is level-trigger";
  }
#else
  LOG_TRACE << "poll(2) can't set to edge-trigger mode, the only mode is level-trigger";
#endif
}

bool EventLoop::IsEdgeTriggerMode() const noexcept
{ 
#ifdef ENABLE_EPOLL
  if (!is_poller_) {
    auto ptr = kanon::down_pointer_cast<Epoller>(poller_.get());
    KANON_ASSERT(ptr, "This must be a Epoller*");

    return ptr->IsEdgeTriggerMode();
  }
  
  return false;
#else
  return false;
#endif
}

} // namespace kanon
