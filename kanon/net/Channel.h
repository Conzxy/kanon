#ifndef KANON_CHANNEL_H
#define KANON_CHANNEL_H

// obtain POLLRDHUP
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif 

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/log/Logger.h"
#include "kanon/time/TimeStamp.h"
//#include "kanon/net/EventLoop.h"

#include <functional>
#include <string>
#include <poll.h>

#ifdef KANON_ENABLE_EPOLL
#include <sys/epoll.h>
static_assert(EPOLLIN  == POLLIN,  "EPOLLIN should equal to POLLIN");
static_assert(EPOLLOUT == POLLOUT, "EPOLLOUT should equal to POLLOUT");
static_assert(EPOLLPRI == POLLPRI, "EPOLLPRI should equal to POLLRI");
#endif

namespace kanon {

class EventLoop;

/**
 * @brief events dispatching
 */
class Channel {
	// use enum class is not a better choice
	// should be compatible with C API
	
	//@see man poll and epoll
	enum Event {
		ReadEvent = POLLIN | POLLPRI,
		WriteEvent = POLLOUT,
		NoneEvent = 0
	};
	
public:
	typedef std::function<void(TimeStamp)> ReadEventCallback;
	typedef std::function<void()> EventCallback;
		
public:
	Channel(EventLoop* loop, int fd)
		: fd_{ fd }
		, events_{ 0 }
		, revents_{ 0 }
		, index_{ -1 }
		, log_hup_{ true }
		, events_handling_{ false }
		, loop_{ loop }
	{ 
		LOG_TRACE << "Channel fd = " << fd_ << " created";
	}
	
	int fd() const KANON_NOEXCEPT { return fd_; }
	int events() const KANON_NOEXCEPT { return events_; }

	std::string events2String() const KANON_NOEXCEPT { return ev2String(events_); }
	std::string revents2String() const KANON_NOEXCEPT { return ev2String(revents_); }

	void enableReading() KANON_NOEXCEPT {
		events_ |= Event::ReadEvent;
		update();
	}
	
	void enableWriting() KANON_NOEXCEPT {
		events_ |= Event::WriteEvent;
		update();
	}
	
	void disableReading() KANON_NOEXCEPT {
		events_ |= ~Event::ReadEvent;
		update();
	}

	void disableWriting() KANON_NOEXCEPT {
		events_ |= ~Event::WriteEvent;
		update();
	}
	
	bool isReading() const KANON_NOEXCEPT {
		return events_ & Event::ReadEvent;
	}

	bool isWriting() const KANON_NOEXCEPT {
		return events_ & Event::WriteEvent;
	}
	
	bool isNoneEvent() const KANON_NOEXCEPT {
		return events_ == 0;
	}

	void disableAll() KANON_NOEXCEPT {
		events_ = Event::NoneEvent;
		update();
	}
	
	void remove() KANON_NOEXCEPT;	

	// callback is used here only
	void setReadCallback(ReadEventCallback cb) { read_callback_ = std::move(cb); }
	void setWriteCallback(EventCallback cb) { write_callback_ = std::move(cb); }
	void setErrorCallback(EventCallback cb) { error_callback_ = std::move(cb); }
	void setCloseCallback(EventCallback cb) { close_callback_ = std::move(cb); }
	
	int index() KANON_NOEXCEPT { return index_; }
	void setIndex(int index) KANON_NOEXCEPT { index_ = index; }
	void setRevents(int event) KANON_NOEXCEPT { revents_ = event; }
	
	void setLogHup(bool flag) KANON_NOEXCEPT { log_hup_ = flag; }
	void handleEvents(TimeStamp receive_time);

private:
	static std::string ev2String(int ev);

	void update();
private:
	int fd_;
	int events_;
	int revents_;

	int index_;
	ReadEventCallback read_callback_;
	EventCallback write_callback_;
	EventCallback close_callback_;
	EventCallback error_callback_;

	bool log_hup_;
	bool events_handling_;
	EventLoop* loop_;
};

} // namespace kanon

#endif // KANON_CHANNEL_H
