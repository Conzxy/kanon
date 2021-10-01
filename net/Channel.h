#ifndef KANON_CHANNEL_H
#define KANON_CHANNEL_H

// obtain POLLRDHUP
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif 

#include "kanon/util/noncopyable.h"

#include <functional>
#include <string>
#include <poll.h>

namespace kanon {

class EventLoop;

/*
 * @brief events dispatching
 */
class Channel : noncopyable {
	enum Event {
		ReadEvent = POLLIN | POLLPRI,
		WriteEvent = POLLOUT,
		NoneEvent = 0
	};
	
public:
	typedef std::function<void()> EventCallback;
	
public:
	Channel(EventLoop* loop, int fd)
		: fd_{ fd }
		, loop_{ loop }
	{ }
	
	int fd() const noexcept { return fd_; }
	int events() const noexcept { return events_; }

	std::string events2String() const noexcept { return ev2String(fd_, events_); }
	std::string revents2String() const noexcept { return ev2String(fd_, revents_); }

	void enableReading() noexcept {
		events_ |= Event::ReadEvent;
		update();
	}
	
	void enableWriting() noexcept {
		events_ |= Event::WriteEvent;
		update();
	}
	
	void disableReading() noexcept {
		events_ |= ~Event::ReadEvent;
		update();
	}

	void disableWriting() noexcept {
		events_ |= ~Event::WriteEvent;
		update();
	}
	
	bool isReading() const noexcept {
		return events_ & Event::ReadEvent;
	}

	bool isWriting() const noexcept {
		return events_ & Event::WriteEvent;
	}
	
	bool isNoneEvent() const noexcept {
		return events_ == 0;
	}

	void disableAll() noexcept {
		events_ = Event::NoneEvent;
		update();
	}
	
	// callback is used here only
	void set_read_callback(EventCallback cb) { read_callback_ = std::move(cb); }
	void set_write_callback(EventCallback cb) { write_callback_ = std::move(cb); }
	void set_error_callback(EventCallback cb) { error_callback_ = std::move(cb); }
	void close_callback(EventCallback cb) { close_callback_ = std::move(cb); }

	void set_revents(int event) noexcept { revents_ = event; }
	
	void handleEvents();
private:
	static std::string ev2String(int fd, int ev);

	void update() {
		loop_->updateChannel(this);
	}

private:
	int fd_;
	int events_;
	int revents_;
	
	EventCallback read_callback_;
	EventCallback write_callback_;
	EventCallback close_callback_;
	EventCallback error_callback_;

	bool log_hup_;
	bool events_handling_;
	EventLoop* loop_;
};

} // namespace kanon

#endif // KANON_CHANNEL_H
