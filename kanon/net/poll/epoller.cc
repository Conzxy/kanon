#include "kanon/net/poll/epoller.h"

#include <unistd.h>

#include "kanon/net/event_loop.h"

namespace kanon {

// since no need to maintain event table
// we can reuse Channel::index_ to indicate event status in kerner events table
enum EventStatus {
  kNew = -1, // event is never added to events table
  kAdded = 1, // event has added
  kDeleted, // event has removed
};

namespace detail {

static inline int CreateEpollFd() noexcept {
  // since linux 2.6.8, size argument is ignored for epoll_create()
  // on the contrary, epoll_create1() accept a flag argument
  // only one flag: EPOLL_CLOEXEC
  int ret = ::epoll_create1(EPOLL_CLOEXEC);

  if (ret < 0) {
      LOG_SYSFATAL << "epoll_create1() error occurred";
  }

  return ret;
}

} // namespace detail

static constexpr int kEventInitNums = 16;

Epoller::Epoller(EventLoop* loop)
  : PollerBase{ loop }
  , epoll_fd_{ detail::CreateEpollFd() }
  , events_{ kEventInitNums }
{
  LOG_TRACE << "Epoller is created";
}

Epoller::~Epoller() noexcept {
  ::close(epoll_fd_);
  LOG_TRACE << "Epoller is destroyed";
}

TimeStamp Epoller::Poll(int ms, ChannelVec& active_channels) {
  AssertInThread();

  int ev_nums = ::epoll_wait(epoll_fd_, 
                             events_.data(),
                             static_cast<int>(events_.size()), // maxsize
                             ms);

  int saved_errno = errno; 
  TimeStamp now{ TimeStamp::Now() };

  if (ev_nums > 0) {
    LOG_TRACE << ev_nums << " events are ready";
    FillActiveChannels(ev_nums, active_channels);
    
    // since epoll_wait does not expand space and 
    // not use any modifier member function of std::vector
    // so size is not modified automacally
    // ==> should use resize() instead of reserve()
    if (static_cast<int>(events_.size()) == ev_nums) {
      events_.resize(ev_nums << 1);
    }
  } else if (ev_nums == 0) {
    LOG_TRACE << "none events ready";
  } else {
    // use saved_errno to avoid misunderstand error
    if (saved_errno != EINTR) {
      errno = saved_errno;
      LOG_SYSERROR << "epoll_wait() error occurred";
    }
  }

  return now;
}

void Epoller::FillActiveChannels(int ev_nums, 
                                 ChannelVec& active_channels) noexcept {  
  assert(ev_nums <= static_cast<int>(events_.size()));
  
  for (int i = 0; i < ev_nums; ++i) {
    // we use the data.ptr so that no need to look up in channels_map_
    auto channel = reinterpret_cast<Channel*>(events_[i].data.ptr);
    assert(!channel->IsNoneEvent());

    int fd = channel->GetFd();KANON_UNUSED(fd);

    channel->SetRevents(events_[i].events);
    active_channels.emplace_back(channel);
  }
}

void Epoller::UpdateChannel(Channel* ch) {
  AssertInThread();

  int index = ch->GetIndex();

  if (index == kNew) {
    UpdateEpollEvent(EPOLL_CTL_ADD, ch);
    ch->SetIndex(kAdded);
  } else { // ch->GetIndex() = kAdded
    if (ch->IsNoneEvent()) {
      UpdateEpollEvent(EPOLL_CTL_DEL, ch);
      ch->SetIndex(kNew);
    } else { 
      UpdateEpollEvent(EPOLL_CTL_MOD, ch);
    }
  }
}

namespace detail {
static inline char const* Op2Str(int op) noexcept {
  switch (op) {
  case EPOLL_CTL_ADD:
    return " ADD ";
  case EPOLL_CTL_DEL:
    return " DEL ";
  case EPOLL_CTL_MOD:
    return " MOD ";
  default:
    return " Unknown operation ";
  }
}

} // namespace detail

void Epoller::UpdateEpollEvent(int op, Channel* ch) noexcept {
  int fd = ch->GetFd();

  Event ev;

  ev.events = ch->GetEvents();
  if (!ch->IsNoneEvent() && is_et_mode_) {
    ev.events |= EPOLLET;
  }

  LOG_TRACE << "epoll_ctl op =" << detail::Op2Str(op) 
    << " fd: " << fd << " {" << Channel::Ev2String(ev.events) << "}";
  
  // In this way, can get channel accroding to fd in O(1)
  ev.data.ptr = static_cast<void*>(ch);

  if (::epoll_ctl(epoll_fd_, op, ch->GetFd(), &ev)) {
    if (op == EPOLL_CTL_DEL) {
      LOG_SYSERROR << "epoll_ctl() op =" << detail::Op2Str(op)
        << " fd = " << fd;
    } else {
      LOG_SYSFATAL << "epoll_ctl() op =" << detail::Op2Str(op) 
        << " fd = " << fd;
    }
  }
}

void Epoller::RemoveChannel(Channel* ch) {
  AssertInThread();
  
  int fd = ch->GetFd();

  LOG_TRACE << "Remove fd = " << fd;

  auto index = ch->GetIndex();
  
  if (index == kAdded) {
    UpdateEpollEvent(EPOLL_CTL_DEL, ch);
    // set status to kNew, then we can add it again
    ch->SetIndex(kNew);
  }

}

} // namespace kanon
