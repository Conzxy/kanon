#include "poller.h"
#include "kanon/log/logger.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/channel.h"
#include <algorithm>
#include <assert.h>

namespace kanon {

TimeStamp Poller::Poll(int ms, ChannelVec& activeChannels) noexcept {
  Base::AssertInThread();

  auto ret = ::poll(pollfds_.data(), 
            static_cast<nfds_t>(pollfds_.size()), 
            ms);
  TimeStamp now{ TimeStamp::Now() };

  if (ret > 0) {
    // when ret = 0, break the loop to avoid whole poll
    uint32_t ev_num = ret;
    for (auto const& pollfd : pollfds_) {
      if (ev_num == 0)
        break;
      
      if (pollfd.revents > 0) {
        LOG_TRACE << ret << " events are ready";
        
        auto it = channelMap_.find(pollfd.fd);
        assert(it != channelMap_.end());

        auto channel = it->second;
        assert(channel);
        assert(channel->GetFd() == pollfd.fd);

        channel->SetRevents(pollfd.revents);

        activeChannels.emplace_back(channel);
        
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
  Base::AssertInThread();

  auto it = channelMap_.find(ch->GetFd());
 
  if (it == channelMap_.end()) {
    // not exist, to create a new channel in channelMap
    assert(ch->GetIndex() == -1);
    // use hint version of insert is better
    // because new fd is more than or equal all fd in channelMap_
    channelMap_.insert(channelMap_.end(), std::make_pair(ch->GetFd(), ch));
    struct pollfd new_pollfd;
    new_pollfd.fd = ch->GetFd();
    new_pollfd.events = static_cast<short>(ch->GetEvents());
    new_pollfd.revents = 0;

    pollfds_.emplace_back(std::move(new_pollfd)); // although std::move is meaningless since pollfd is POD type
  
    // channel index <==> pollfds size
    ch->SetIndex(pollfds_.size() - 1);
  } else { 
    // channel now exist, update it
    assert(channelMap_[ch->GetFd()] == ch);
    
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
  Base::AssertInThread();

  auto it = channelMap_.find(ch->GetFd());

  if (it != channelMap_.end()) {
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
      
      // iter_swap just swap content which iterator point to
      std::iter_swap(remove_iter, std::prev(pollfds_.end()));
      pollfds_.pop_back();      
        
      // although back fd is negative
      // also need fd to a opposite number
      // channel.fd <==> pollfd.fd
      // if is not interested in these events on fd, set fd to -fd - 1
      // but channel.fd is immutable
      // 
      if (back_fd < 0) {
        back_fd = -back_fd - 1;
      }
      
      channelMap_[back_fd]->SetIndex(index);
    }

    channelMap_.erase(it);
  }
}

} // namespace kanon
