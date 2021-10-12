#include "Poller.h"
#include "kanon/log/Logger.h"
#include "kanon/net/EventLoop.h"
#include "kanon/net/Channel.h"
#include <algorithm>
#include <assert.h>

namespace kanon {

TimeStamp Poller::poll(int ms, ChannelVec& activeChannels) noexcept {
	Base::assertInThread();

	auto ret = ::poll(pollfds_.data(), 
					  static_cast<nfds_t>(pollfds_.size()), 
					  ms);
	TimeStamp now{ TimeStamp::now() };

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
				assert(channel->fd() == pollfd.fd);

				channel->set_revents(pollfd.revents);

				activeChannels.emplace_back(channel);
				
				--ev_num; 
			}
		}
	} else if (ret == 0) {
		LOG_TRACE << "none events are ready";
	} else { // ret < 0
		LOG_SYSERROR << "poll() error occurred";
	}

	return now;
}

void Poller::updateChannel(Channel* ch) {
	Base::assertInThread();

	auto it = channelMap_.find(ch->fd());
	
	if (it == channelMap_.end()) {
		// not exist, to create a new channel in channelMap
		assert(ch->index() == -1);
		channelMap_[ch->fd()] = ch;
		struct pollfd new_pollfd;
		new_pollfd.fd = ch->fd();
		new_pollfd.events = static_cast<short>(ch->events());
		new_pollfd.revents = 0;

		pollfds_.emplace_back(std::move(new_pollfd)); // although std::move is meaningless since pollfd is POD type
	
		// channel index <==> pollfds size
		ch->set_index(pollfds_.size() - 1);
	} else { 
		// channel now exist, update it
		assert(channelMap_[ch->fd()] == ch);
		
		auto index = ch->index();
		assert(index != -1 && index <= static_cast<int>(pollfds_.size()));
		auto& now_pollfd = pollfds_[index];
		
		assert(now_pollfd.fd == ch->fd());
	
		if (ch->isNoneEvent()) {
			now_pollfd.fd = -now_pollfd.fd - 1;
		}
		now_pollfd.events = static_cast<short>(ch->events());
		now_pollfd.revents = 0;
	}
}

void Poller::removeChannel(Channel* ch) {
	Base::assertInThread();

	auto it = channelMap_.find(ch->fd());

	if (it != channelMap_.end()) {
		auto channel = it->second;
		
		if (channel->fd() == pollfds_.back().fd) {
			// ch is the last pollfd
			// just pop back
			pollfds_.pop_back();
		} else {
			// ========================
			// |    | fd |    | back |
			// ========================
			// swap fd and back, then set back index to fd.index
			auto index = ch->index();
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
			
			channelMap_[back_fd]->set_index(index);
		}

		channelMap_.erase(it);
	}
}

} // namespace kanon
