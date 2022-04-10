#include "kanon/net/poll/poller.h"

#include <algorithm>
#include <assert.h>

#include "kanon/log/logger.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/channel.h"

namespace kanon {

Poller::Poller(EventLoop* loop)
  : PollerBase(loop)
{
  LOG_TRACE << "Poller is created";
}

Poller::~Poller() noexcept
{
  LOG_TRACE << "Poller is destroyed";
}

TimeStamp Poller::Poll(int ms, ChannelVec& active_channels) noexcept {
  AssertInThread();

  auto ret = ::poll(pollfds_.data(), 
                    static_cast<nfds_t>(pollfds_.size()), 
                    ms);

  TimeStamp now{ TimeStamp::Now() };

  if (ret > 0) {
    LOG_TRACE << ret << " events are ready";
    uint32_t ev_num = ret;

    for (auto const& pollfd : pollfds_) {
      if (ev_num == 0)
        break;
      
      if (pollfd.revents > 0) {
        
        auto it = channels_map_.find(pollfd.fd);
        assert(it != channels_map_.end());

        auto channel = it->second;
        assert(channel);
        assert(channel->GetFd() == pollfd.fd);

        channel->SetRevents(pollfd.revents);

        active_channels.emplace_back(channel);
        
        --ev_num; 
      }
    }
  } else if (ret == 0) {
    LOG_TRACE << "none events are ready";
  } else { // ret < 0
    LOG_SYSERROR << "Poll() error occurred";
  }

  return now;
}

void Poller::UpdateChannel(Channel* ch) {
  AssertInThread();

  auto it = channels_map_.find(ch->GetFd());
 
  if (it == channels_map_.end()) {
    // not exist, to create a new channel in channelMap
    assert(ch->GetIndex() == -1);
    // use hint version of insert is better
    // because new fd is more than or equal all fd in channels_map_
    channels_map_.emplace_hint(channels_map_.end(), 
                               ch->GetFd(), ch);
    struct pollfd new_pollfd;
    new_pollfd.fd = ch->GetFd();
    new_pollfd.events = static_cast<short>(ch->GetEvents());
    new_pollfd.revents = 0;

    pollfds_.emplace_back(std::move(new_pollfd)); // although std::move is meaningless since pollfd is POD type
  
    // channel index <==> pollfds size
    ch->SetIndex(pollfds_.size() - 1);
  } else { 
    // channel now exist, update it
    assert(it->second == ch);
    
    auto index = ch->GetIndex();
    assert(index != -1 && index <= static_cast<int>(pollfds_.size()));
    auto& now_pollfd = pollfds_[index];
    
    assert(now_pollfd.fd == ch->GetFd());
  
    if (ch->IsNoneEvent()) {
      now_pollfd.fd = -now_pollfd.fd - 1;
    }

    now_pollfd.events = static_cast<short>(ch->GetEvents());
    now_pollfd.revents = 0;
  }
}

void Poller::RemoveChannel(Channel* ch) {
  AssertInThread();

  auto it = channels_map_.find(ch->GetFd());

  if (it != channels_map_.end()) {
    auto channel = it->second;
    
    if (channel->GetFd() == pollfds_.back().fd) {
      // ch is the last pollfd
      // just pop back
      pollfds_.pop_back();
    } else {
      // ========================
      // |    | fd |    | back |
      // ========================
      // swap fd and back, then set back index to fd.index
      auto index = ch->GetIndex();
      assert(index != -1 && index < static_cast<int>(pollfds_.size()));
      
      auto back_fd = pollfds_.back().fd;

      // use iterator algorithm to replace container type conveniently
      auto remove_iter = pollfds_.begin();
      std::advance(remove_iter, index);
      
      std::swap(*remove_iter, pollfds_.back());
      pollfds_.pop_back();      
        
      // Although back fd is negative
      // also need fd to a opposite number
      // channel.fd <==> pollfd.fd
      // if is not interested in these events on fd, set fd to -fd - 1
      // but channel.fd is immutable
      // 
      if (back_fd < 0) {
        back_fd = -back_fd - 1;
      }
      
      channels_map_[back_fd]->SetIndex(index);
    }

    channels_map_.erase(it);
  }
}

} // namespace kanon
