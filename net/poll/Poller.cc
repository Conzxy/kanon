#include "Poller.h"
#include "kanon/log/Logger.h"
#include "kanon/net/Channel.h"


namespace kanon {

int POLLTIMEOUT = 5 * 1000;

TimeStamp Poller::poll(int ms, ChannelVec& activeChannels) noexcept {
	auto ret = ::poll(pollfds_.data(), 
					  static_cast<nfds_t>(pollfds_.size()), 
					  ms);
	TimeStamp now{ TimeStamp::now() };

	if (ret > 0) {
		for (auto const& pollfd : pollfds_) {
			LOG_TRACE << ret << "events are ready";
			
			auto it = channelMap_.find(pollfd.fd);
			assert(it != channelMap_.end());
			
			auto channel = it->second;
			assert(channel);

			channel->set_revents(pollfd.fd);

			activeChannels.emplace_back(channel);
		}
	} else if (ret == 0) {
		LOG_TRACE << "none events are ready";
	} else { // ret < 0
		LOG_SYSERROR << "poll() error";
	}

	return now;
}

void Poller::updateChannel(Channel* ch) {
	auto it = channelMap_.find(ch->fd());
	
	if (it == channelMap_.end()) {
		// not exist, to create a new channel in channelMap
		channelMmap_[ch->fd()
	}
}
} // namespace kanon
