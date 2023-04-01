#include "kanon/util/platform_macro.h"
#ifdef KANON_ON_UNIX
#include <sys/eventfd.h>
#include <unistd.h>
#include "kanon/net/poll/poller.h"
#include "kanon/net/poll/epoller.h"
#elif defined(KANON_ON_WIN)
#include <winsock2.h>
#include <ioapiset.h>
#include "kanon/net/poll/iocp_poller.h"
#endif

#include "kanon/net/sock_api.h"
#include "kanon/util/ptr.h"
#include "kanon/net/event_loop.h"

#include <assert.h>
#include "kanon/thread/current_thread.h"
#include "kanon/util/ptr.h"
#include "kanon/util/time_stamp.h"
#include "kanon/util/macro.h"
#include "kanon/log/logger.h"

#include "kanon/net/timer/timer_queue.h"
#include "kanon/net/channel.h"
#include "kanon/net/macro.h"

using namespace kanon::process;

// To avoid swap out this process to disk
// Don't set timeout parameter of poll()/epoll_wait()
// to negative or zero
#ifdef KANON_ON_UNIX
#define POLLTIME -1 // 10s
#elif defined(KANON_ON_WIN)
#define POLLTIME INFINITE
#endif

namespace kanon {

namespace detail {

#ifdef KANON_ON_UNIX
/**
 * Event fd API
 */
static KANON_INLINE int CreateEventFd() KANON_NOEXCEPT
{
  int evfd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

  LOG_TRACE_KANON << "eventfd: " << evfd << " created";

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
static KANON_INLINE void ReadEventFd(int evfd) KANON_NOEXCEPT
{
  uint64_t dummy;
  if (sizeof dummy != ::read(evfd, &dummy, sizeof dummy))
    LOG_SYSERROR << "ReadEventFd() error occurred";
}

static KANON_INLINE void WriteEventFd(int evfd) KANON_NOEXCEPT
{
  uint64_t dummy = 1;
  if (sizeof dummy != ::write(evfd, &dummy, sizeof dummy))
    LOG_SYSERROR << "WriteEventFd() error occurred";
}
#endif

} // namespace detail

EventLoop::EventLoop()
  : EventLoop(false)
{
}

EventLoop::EventLoop(bool is_poller)
#if KANON___THREAD_DEFINED
  : owner_thread_id_{(PId)CurrentThread::t_tid}
#else
  : owner_thread_id_{(PId)CurrentThread::GetTid()}
#endif
  , looping_{false}
  , quit_{false}
  , calling_functors_{false}
  , is_poller_{is_poller}
#ifdef KANON_ON_UNIX
#ifdef ENABLE_EPOLL
  , poller_{is_poller_ ? static_cast<PollerBase *>(new Poller(this))
                       : static_cast<PollerBase *>(new Epoller(this))}
#else
  , poller_(std::make_unique<Poller>(this))
#endif
#elif defined(KANON_ON_WIN)
  , poller_(std::make_unique<IocpPoller>(this))
#endif
// CreateEventfd() is don't throw exception, don't wrong exception-safety
#ifdef KANON_ON_UNIX
  , ev_channel_{kanon::make_unique<Channel>(this, detail::CreateEventFd())}
#elif defined(KANON_ON_WIN)
  , ev_channel_{kanon::make_unique<Channel>(
        this, sock::CreateNonBlockAndCloExecSocket(false))}
#endif
  , timer_queue_{kanon::make_unique<TimerQueue>(this)}
{

  ev_channel_->SetReadCallback([this](TimeStamp receive_time) {
    LOG_TRACE_KANON << "EventFd receive_time: "
                    << receive_time.ToFormattedString(true);
#ifdef KANON_ON_UNIX
    this->EvRead();
#endif
  });

#ifdef KANON_ON_UNIX
  ev_channel_->SetWriteCallback([this]() {
    this->Wakeup();
  });
#endif

  ev_channel_->EnableReading();
  LOG_TRACE_KANON << "EventLoop " << this << " created";
}

EventLoop::~EventLoop()
{
  // No need to Quit() explicitly here:
  // 1. If in loop, call it by user instead of library
  // 2. If not in loop, call it by EventLoopThread so
  //    it don't considered by user.
  LOG_TRACE_KANON << "EventLoop " << this << " destroyed";

  assert(!looping_);
}

void EventLoop::StartLoop()
{
  assert(!looping_);
  AssertInThread();

  looping_ = true;

  LOG_TRACE_KANON << "EventLoop " << this << " loop start";

  std::vector<Channel *> activeChannels;

  while (!quit_) {
    auto receive_time = poller_->Poll(POLLTIME, activeChannels);

    for (auto &channel : activeChannels) {
      channel->HandleEvents(receive_time);
    }

    CallFunctors();

    activeChannels.clear();
  }

  LOG_TRACE_KANON << "EventLoop " << this << " loop stop";

  looping_ = false;
}

void EventLoop::RunInLoop(FunctorCallback cb)
{
  if (IsLoopInThread()) {
    try {
      cb();
    }
    catch (std::exception const &ex) {
      LOG_ERROR << "std::exception(including its derived class) is caught in "
                   "RunInLoop()";
      LOG_ERROR << "Reason: " << ex.what();
      KANON_RETHROW;
    }
    catch (...) {
      LOG_ERROR << "Unknown exception is caught in RunInLoop()";
      KANON_RETHROW;
    }
  } else {
    this->QueueToLoop(std::move(cb));
  }
}

void EventLoop::QueueToLoop(FunctorCallback cb)
{
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

  // If call QueueToLoop() in the "call functor" phase(i.e. phase 3), we can't
  // ensure the next loop don't do a empty poll(and block), should Wakeup()
  // eventfd to avoid it. The other two phase it OK, since phase 3 in after.
  // e.g.
  // Sending file in pipeline way.
  // If the first write is not complete, we need to write it again.
  // Fortunatly, kanon provide WriteCompleteCallback can implemete
  // it. We first register it in phase2(Suppose we send it in
  // ConnectionCallback) then WriteCompleteCallback continue write and register
  // it if not complete, but this is in the phase3, if we don't Wakeup(), the
  // next loop must be blocked. \see example/file_transfer/client.cc
  if (!IsLoopInThread() || calling_functors_) {
    Wakeup();
  }
}

void EventLoop::UpdateChannel(Channel *ch) { poller_->UpdateChannel(ch); }

void EventLoop::RemoveChannel(Channel *ch) { poller_->RemoveChannel(ch); }

void EventLoop::CallFunctors()
{
  if (quit_) {
    return;
  }

  decltype(functors_) functors;
  {
    MutexGuard dummy{lock_};
    functors.swap(functors_);
  }

  calling_functors_ = true;
  for (auto &func : functors) {
    try {
      if (KANON_LIKELY(func)) {
        func();
      }
    }
    catch (std::exception const &ex) {
      LOG_ERROR << "std::exception caught in CallFunctors()";
      LOG_ERROR << "Reason: " << ex.what();
      calling_functors_ = false;
      KANON_RETHROW;
    }
    catch (...) {
      LOG_ERROR << "Unknown exception caught in CallFunctors()";
      calling_functors_ = false;
      KANON_RETHROW;
    }
  }

  calling_functors_ = false;
}

void EventLoop::EvRead() KANON_NOEXCEPT
{
#ifdef KANON_ON_UNIX
  detail::ReadEventFd(ev_channel_->GetFd());
#endif
}

void EventLoop::Wakeup() KANON_NOEXCEPT
{
  // FIXME Thread-safe
#ifdef KANON_ON_UNIX
  detail::WriteEventFd(ev_channel_->GetFd());
#elif defined(KANON_ON_WIN)
  ::PostQueuedCompletionStatus(
      kanon::down_pointer_cast<IocpPoller>(poller_.get())->completion_port(), 0,
      (ULONG_PTR)ev_channel_.get(), NULL);
  ev_channel_->EnableWriting();
#endif
}

void EventLoop::Quit() KANON_NOEXCEPT
{
  if (quit_) return;
  quit_ = true;

  LOG_DEBUG_KANON << "EventLoop is quiting";

  // If in the IO thread, call Wakeup() in Quit() is not necessary,
  // because it only few cases can call Quit() successfully
  // * In the loop, the only choice is timer event, but it is not blocking
  // * In the other thread, do an asynchronous call, need call Wakeup() to
  //   avoid empty poll like QueueToLoop()
  if (!this->IsLoopInThread()) this->Wakeup();
}

TimerId EventLoop::RunAt(TimerCallback cb, TimeStamp expiration)
{
  LOG_DEBUG << "expiration = " << expiration.ToFormattedString();
  return timer_queue_->AddTimer(std::move(cb), expiration,
                                0.0);
}

TimerId EventLoop::RunEvery(TimerCallback cb, TimeStamp expiration,
                            double interval)
{
  return timer_queue_->AddTimer(std::move(cb), expiration,
                                interval);
}

void EventLoop::CancelTimer(TimerId timer_id)
{
  timer_queue_->CancelTimer(timer_id);
}

void EventLoop::AbortNotInThread() KANON_NOEXCEPT
{
  LOG_FATAL << "The policy of \"One Loop Per Thread\" has destroyed!";
}

void EventLoop::SetEdgeTriggerMode() KANON_NOEXCEPT
{
#ifdef ENABLE_EPOLL
  if (!is_poller_) {
    auto ptr = kanon::down_pointer_cast<Epoller>(poller_.get());
    KANON_ASSERT(ptr, "This must be a Epoller*");
    ptr->SetEdgeTriggertMode();
    LOG_TRACE_KANON << "The Poller will working in edge-trigger mode";
  } else {
    LOG_TRACE_KANON << "poll(2) can't set to edge-trigger mode, the only mode "
                       "is level-trigger";
  }
#else
  LOG_TRACE_KANON << "poll(2) can't set to edge-trigger mode, the only mode is "
                     "level-trigger";
#endif
}

bool EventLoop::IsEdgeTriggerMode() const KANON_NOEXCEPT
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

#if !KANON___THREAD_DEFINED
bool EventLoop::IsLoopInThread() KANON_NOEXCEPT
{
  return CurrentThread::GetTid() == owner_thread_id_;
}
#endif

} // namespace kanon
