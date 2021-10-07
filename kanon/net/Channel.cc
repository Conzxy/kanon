#include "kanon/net/EventLoop.h"
#include "kanon/log/Logger.h"

#include <stdio.h>

namespace kanon {

std::string Channel::ev2String(int ev) {
	std::string buf;
	buf.reserve(64);

	buf += " :";
	if (ev & POLLIN)
		buf += " IN";
	if (ev & POLLPRI)
		buf += " PRI";
	if (ev & POLLOUT)
		buf += " OUT";
	if (ev & POLLRDHUP)
		buf += " RDHUP";
	// only revents
	if (ev & POLLHUP)
		buf += " HUP";
	if (ev & POLLERR)
		buf += " ERR";
	if (ev & POLLNVAL)
		buf += " NVAL";
	buf += " ";

	return buf; // RVO will use move semantic
}

void Channel::handleEvents() {
	events_handling_ = true;
	
	LOG_TRACE << "channel fd: " << fd_ << revents2String();

	if ((revents_ & POLLHUP) && (revents_ & POLLIN)) {
		if (log_hup_) {
			LOG_WARN << "fd: " << fd_ << " POLLHUP happened";
		}

		if (close_callback_) close_callback_();
	}
	
	// fd not open
	// also a error(after if branch log error)
	if (revents_ & POLLNVAL) {
		LOG_WARN << "fd: " << fd_ << " POLLNVAL happend";	
	}
	
	if (revents_ & (POLLERR | POLLNVAL)) {
		if (error_callback_) error_callback_();
	}

	// RDHUP indicate peer half-close
	// since previous HUP
	if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
		if (read_callback_) read_callback_();
	}

	if (revents_ & POLLOUT) {
		if (write_callback_) write_callback_();
	}

	events_handling_ = false;
}

void Channel::update() {
	LOG_TRACE << "Channel fd: " << fd_ << this->events2String();
	loop_->updateChannel(this);
}

} // namespace kanon
