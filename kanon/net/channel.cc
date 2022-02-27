#include "kanon/net/event_loop.h"
#include "kanon/log/logger.h"

#include "kanon/net/channel.h"
#include "kanon/util/macro.h"

#include <stdio.h>

namespace kanon {

Channel::Channel(EventLoop* loop, int fd)
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

Channel::~Channel() noexcept
{
  // In the handling events phase, close_callback_ remove the connection(i.e. channel),
  // it is unsafe, we should remove it in next phase(calling functor phase)
  KANON_ASSERT(!events_handling_, 
    "Event is handling when Channel is destoyed."
    "Advise: You should call EventLoop::QueueToLoop() to delay remove");
}

void Channel::HandleEvents(TimeStamp receive_time) {
  events_handling_ = true;
  
  LOG_TRACE << "Event Receive Time: " << receive_time.ToFormattedString(true);  
  LOG_TRACE << "fd = " << fd_ << ", revent(result event) : {" << Revents2String() << "}";
  

  /**
   * POLLHUP indicates the connection is closed in two direction
   * @see https://stackoverflow.com/questions/56177060/pollhup-vs-pollrdhup
   * But according the manpage of poll
   * POLLHUP also can read until 0
   * So !revents_ & POLLIN
   */
  if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
    if (log_hup_) {
      LOG_WARN << "fd = " << fd_ << " POLLHUP happened";
    }

    if (close_callback_) close_callback_();
  }
  
  // Fd not open
  // also a error(after if branch log error)
  if (revents_ & POLLNVAL) {
    LOG_WARN << "fd = " << fd_ << " POLLNVAL(fd not open) happend";  
  }
  
  if (revents_ & (POLLERR | POLLNVAL)) {
    if (error_callback_) error_callback_();
  }

  // When revents_ == POLLIN, process message except EOF(FIN or RESET)
  // RDHUP indicates peer half-close(but we don't distinguish, also close)
  // FIXME If peer shutdown, we need skip write event
  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (read_callback_) read_callback_(receive_time);
  }

  if (revents_ & POLLOUT) {
    if (write_callback_) write_callback_();
  }

  events_handling_ = false;
}

void Channel::Update() {
  loop_->UpdateChannel(this);
}

void Channel::Remove() noexcept {
  loop_->RemoveChannel(this);
}

std::string Channel::ev2String(int ev) {
  std::string buf;
  buf.reserve(32);

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

} // namespace kanon