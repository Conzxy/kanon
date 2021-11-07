#include "Epoller.h"
#include "kanon/net/EventLoop.h"

#include <unistd.h>

namespace kanon {

// since no need to maintain event table
// we can reuse Channe::index_ to indicate event status in kerner events table
enum EventStatus {
    kNew = -1, // event is never added to events table
    kAdded = 1, // event has added
    kDeleted = 2, // event has been added before but deleted now
};

namespace detail {
    static inline int createEpollfd() KANON_NOEXCEPT {
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

#define EV_INIT_NUMS 16

Epoller::Epoller(EventLoop* loop)
    : Base{ loop }
    , epoll_fd_{ detail::createEpollfd() }
    , events_{ EV_INIT_NUMS }
{
    LOG_TRACE << "Epoller created";
}

Epoller::~Epoller() KANON_NOEXCEPT {
	::close(epoll_fd_);
}

TimeStamp 
Epoller::poll(int ms, ChannelVec& activeChannels) {
    Base::assertInThread();

    int ev_nums = ::epoll_wait(epoll_fd_, 
                               events_.data(),
                               static_cast<int>(events_.size()), // maxsize
                               ms);
    
    int saved_errno = errno; 
    TimeStamp now{ TimeStamp::now() };

    if (ev_nums > 0) {
        LOG_TRACE << ev_nums << " events are ready";
        fillActiveChannels(ev_nums, activeChannels);
        
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

// typedef union epoll_data {
//	void *ptr;
//	int fd;
//	uint32_t u32;
//	uint64_t u64;
// } epoll_data_t;
//
// struct epoll_event {
// 	uint32_t events; // Epoll events
// 	epoll_data_t data; // user data variable
// };
void 
Epoller::fillActiveChannels(int ev_nums, 
                            ChannelVec& activeChannels) KANON_NOEXCEPT {  
    assert(ev_nums <= static_cast<int>(events_.size()));
    
    for (int i = 0; i < ev_nums; ++i) {
		// we use the data.ptr so that no need to look up in channelMap_
        auto channel = reinterpret_cast<Channel*>(events_[i].data.ptr);
        assert(!channel->isNoneEvent());

        int fd = channel->fd();KANON_UNUSED(fd);
        assert(channelMap_.find(fd) != channelMap_.end()); 
        assert(channelMap_[fd] == channel);

        channel->setRevents(events_[i].events);
        activeChannels.emplace_back(channel);
    }
}

void
Epoller::updateChannel(Channel* ch) {
    Base::assertInThread();

    int fd = ch->fd();
	int index = ch->index();

    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            assert(channelMap_.find(fd) == channelMap_.end());
            channelMap_.insert(channelMap_.end(), std::make_pair(fd, ch));
        } else {
            assert(channelMap_.find(fd) != channelMap_.end());
            assert(channelMap_[fd] == ch);
        } 

        updateEpollEvent(EPOLL_CTL_ADD, ch);
        ch->setIndex(kAdded);
    } else { // ch->index() = kAdded
        assert(channelMap_.find(fd) != channelMap_.end());
        assert(channelMap_[fd] == ch);
    
        if (ch->isNoneEvent()) {
            updateEpollEvent(EPOLL_CTL_DEL, ch);
            ch->setIndex(kDeleted);
        } else { 
            updateEpollEvent(EPOLL_CTL_MOD, ch);
        }
    }
}

namespace detail {
static inline char const* op2Str(int op) KANON_NOEXCEPT {
    switch (op) {
    case EPOLL_CTL_ADD:
        return " ADD ";
    case EPOLL_CTL_DEL:
        return " DEL ";
    case EPOLL_CTL_MOD:
        return " MOD ";
    default:
        assert(false && "error op");
        return " Unknown operation ";
    }
}

} // namespace detail

void
Epoller::updateEpollEvent(int op, Channel* ch) KANON_NOEXCEPT {
    int fd = ch->fd();

    LOG_TRACE << "epoll_ctl op =" << detail::op2Str(op) 
        << " fd: " << fd << " {" << ch->events2String() << "}";

    Event ev;
    ev.events = ch->events();
    ev.data.ptr = static_cast<void*>(ch);

    if (::epoll_ctl(epoll_fd_, op, ch->fd(), &ev)) {
        if (op == EPOLL_CTL_DEL) {
            LOG_SYSERROR << "epoll_ctl() op =" << detail::op2Str(op)
                << " fd = " << fd;
        } else {
            LOG_SYSFATAL << "epoll_ctl() op =" << detail::op2Str(op) 
                << " fd = " << fd;
        }
    }
}

void
Epoller::removeChannel(Channel* ch) {
    Base::assertInThread();
    
    int fd = ch->fd();

    LOG_TRACE << "remove fd: " << fd;

    assert(channelMap_.find(fd) != channelMap_.end());
    assert(channelMap_[fd] == ch);
    
    auto index = ch->index();
    assert(index == kAdded || index == kDeleted);
    
    if (index == kAdded) {
        updateEpollEvent(EPOLL_CTL_DEL, ch);
    }

    auto n = channelMap_.erase(fd);
	assert(n == 1);
    KANON_UNUSED(n); 

    // ch is a new channel which not in channelMap_ 
	// set status to kNew, then we can readd it
    ch->setIndex(kNew);
}

} // namespace kanon