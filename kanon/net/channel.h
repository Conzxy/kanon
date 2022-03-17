#ifndef KANON_CHANNEL_H
#define KANON_CHANNEL_H

// Obtain POLLRDHUP
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif 

#include <functional>
#include <string>
#include <poll.h>

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/time_stamp.h"
#include "kanon/log/logger.h"

#include "kanon/net/macro.h"

#ifdef ENABLE_EPOLL
#include <sys/epoll.h>
static_assert(EPOLLIN  == POLLIN,  "EPOLLIN should equal to POLLIN");
static_assert(EPOLLOUT == POLLOUT, "EPOLLOUT should equal to POLLOUT");
static_assert(EPOLLPRI == POLLPRI, "EPOLLPRI should equal to POLLRI");
#endif

namespace kanon {

class EventLoop;

/**
 * Represents file descripter and events.
 * Throught the API, you can (un)register event that you interest.
 * This is also a events dispatcher according actual events happend.
 * @note Internal class
 */
class Channel {

  // Use enum class is not a better choice,
  // we should be compatible with C API
  //@see man poll and epoll
  enum Event {
    ReadEvent = POLLIN | POLLPRI,
    WriteEvent = POLLOUT,
    NoneEvent = 0
  };
  
public:
  using ReadEventCallback = std::function<void(TimeStamp)>;
  using EventCallback     = std::function<void()>;
    
public:
  Channel(EventLoop* loop, int fd);
  ~Channel() noexcept;

  int GetFd() const noexcept { return fd_; }
  int GetEvents() const noexcept { return events_; }

  std::string Events2String() const noexcept { return ev2String(events_); }
  std::string Revents2String() const noexcept { return ev2String(revents_); }

  void EnableReading() noexcept {
    events_ |= Event::ReadEvent;
    Update();
  }
  
  void EnableWriting() noexcept {
    events_ |= Event::WriteEvent;
    Update();
  }
  
  void DisableReading() noexcept {
    events_ &= ~Event::ReadEvent;
    Update();
  }

  void DisableWriting() noexcept {
    events_ &= ~Event::WriteEvent;
    Update();
  }
  
  bool IsReading() const noexcept {
    return events_ & Event::ReadEvent;
  }

  bool IsWriting() const noexcept {
    return events_ & Event::WriteEvent;
  }
  
  bool IsNoneEvent() const noexcept {
    return events_ == Event::NoneEvent;
  }

  void DisableAll() noexcept {
    events_ = Event::NoneEvent;
    Update();
  }
  
  void Remove() noexcept;  

  // callback is used here only
  void SetReadCallback(ReadEventCallback cb) { read_callback_ = std::move(cb); }
  void SetWriteCallback(EventCallback cb) { write_callback_ = std::move(cb); }
  void SetErrorCallback(EventCallback cb) { error_callback_ = std::move(cb); }
  void SetCloseCallback(EventCallback cb) { close_callback_ = std::move(cb); }
  
  int GetIndex() noexcept { return index_; }
  void SetIndex(int index) noexcept { index_ = index; }
  void SetRevents(int event) noexcept { revents_ = event; }
  
  void SetLogHup(bool flag) noexcept { log_hup_ = flag; }
  void HandleEvents(TimeStamp receive_time);

private:
  static std::string ev2String(int ev);

  void Update();
private:
  int fd_;
  int events_;
  int revents_;

  int index_;
  ReadEventCallback read_callback_;
  EventCallback write_callback_;
  EventCallback close_callback_;
  EventCallback error_callback_;

  // For logging message when connection is hupping
  bool log_hup_;

  // For assert
  // events_handing_ must be false when dtor is called
  // This force TcpConnection::RemoveConnection to call 
  bool events_handling_;
  EventLoop* loop_;
};

} // namespace kanon

#endif // KANON_CHANNEL_H
