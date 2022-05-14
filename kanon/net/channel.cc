#include "kanon/net/channel.h"

#include <stdio.h>

#include "kanon/log/logger.h"
#include "kanon/util/macro.h"

#include "kanon/net/event_loop.h"

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
  LOG_TRACE_KANON << "Channel fd = " << fd_ << " created";
}

Channel::~Channel() noexcept
{
  // In the handling events phase, close_callback_ remove the connection(i.e. channel),
  // it is unsafe, we should remove it in next phase(calling functor phase)
  KANON_ASSERT(!events_handling_, 
    "Events are being handled when Channel is destoyed.\n"
    "Advice: You should call EventLoop::QueueToLoop() to delay remove");
}

void Channel::HandleEvents(TimeStamp receive_time) {
  events_handling_ = true;
  
  LOG_TRACE_KANON << "Event Receive Time: " << receive_time.ToFormattedString(true);  
  LOG_TRACE_KANON << "fd = " << fd_ << ", revent(result event) : {" << Revents2String() << "}";
  

  /*
   * POLLHUP indicates the connection is closed in two direction
   * i.e. The FIN has been received and sent
   *
   * If POLLIN is set, we can read until 0(FIN->EOF) and close connection
   * Otherwise, close it here
   *
   * \see https://stackoverflow.com/questions/56177060/pollhup-vs-pollrdhup
   */
  if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
    if (log_hup_) {
      LOG_WARN << "fd = " << fd_ << " POLLHUP happened";
    }

    if (close_callback_) close_callback_();
  }
  
  /*
   * To TCP, POLLERR typically indicates RST has been received or sent.
   *   - To server, it is maybe occurred when SO_LINGER is set and timeout is set to 0.
   *     (But to this library, we don't set it). In other case, receive RST segment when
   *     three hand-shake to avoid old duplicate connection(\see RFC 793)
   *   - To client, it is maybe occurred when connect to a nonexistent server or server is 
   *     configured to SO_LINGER/0.  If connect() return acceptable errno except EINPROGRESS,
   *     will close old socket and reconnect to server.
   *     
   * Therefore, the error_callback_ just log error message of socket(see tcp_connection.cc) is ok.
   */
  if (revents_ & (POLLERR | POLLNVAL)) {
    if (revents_ & POLLNVAL) {
      LOG_WARN << "fd = " << fd_ << " POLLNVAL(fd not open) happend";  
    }
    if (error_callback_) error_callback_();
  }

  // When revents_ == POLLIN, process message except FIN or RST
  // RDHUP indicates peer half-close in write direction(but we don't distinguish, also close)
  // So, we can continue receive message
  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (read_callback_) read_callback_(receive_time);
  }

  if (revents_ & POLLOUT) {
    if (write_callback_) write_callback_();
  }

  events_handling_ = false;
}

void Channel::Update() {
  // This must be called in a event loop
  // We just check the eventloop of poller
  // is same with this
  loop_->UpdateChannel(this);
}

void Channel::Remove() noexcept {
  loop_->RemoveChannel(this);
}

std::string Channel::Ev2String(uint32_t ev) {
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
  if (ev & EPOLLET)
    buf += " ET";
  buf += " ";

  return buf; // RVO will use move semantic(11)
}

} // namespace kanon
