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
 * \addtogroup net
 * @{
 */

/**
 * \brief Events Dispatcher
 *
 * As the role of (events) dispatcher in Reactor pattern.
 *
 * Call the APIs can resgister interested events and
 * handler of event.
 *
 * when the events occurred, event loop will notify this
 * through the demultiplexer then this will dispatch 
 * them to corresponseding event handler.
 *
 * \warning Internal class
 */
class Channel {
  /**
   * Use enum class is not a better choice,
   * since it should be compatible with C API
   * \see man 2 poll
   * \see man 2 epoll
  */
  enum Event {
    ReadEvent = POLLIN | POLLPRI,
    WriteEvent = POLLOUT,
    NoneEvent = 0
  };
  
public:
  using ReadEventCallback = std::function<void(TimeStamp)>;
  using EventCallback     = std::function<void()>;
    
  /**
   * \brief Construct a Channel
   * \param fd file descriptor that we want to monitor
   * \warning 
   *   The fd is not managed by this.\n
   *   In kanon, this is maybe socket, timerfd, eventfd
   *   that can be polled(otherwise, will trigger POLLINVAL).\n
   *   i.e. The fd is not released in the dtor.
   */
  Channel(EventLoop* loop, int fd);
  ~Channel() noexcept;
  
 
  //! \name conversion 
  //!@{
 
  //! Convert event to string representation
  static std::string Ev2String(uint32_t ev);

  //! Convert interested event to string representation
  std::string Events2String() const noexcept { return Ev2String(events_); }

  //! Convert receive event to string representation
  std::string Revents2String() const noexcept { return Ev2String(revents_); }

  //!@} // conversion
  
  //! \name event register
  //!@{
 
  //! Start monitoring the read event
  void EnableReading() noexcept {
    events_ |= Event::ReadEvent;
    Update();
  }
  
  //! Start monitoring the write event
  void EnableWriting() noexcept {
    events_ |= Event::WriteEvent;
    Update();
  }
  
  //! Stop monitoring the read event
  void DisableReading() noexcept {
    events_ &= ~Event::ReadEvent;
    Update();
  }

  //! Stop monitoring the write event
  void DisableWriting() noexcept {
    events_ &= ~Event::WriteEvent;
    Update();
  }
  
  //! Stop monitoring any event
  void DisableAll() noexcept {
    events_ = Event::NoneEvent;
    Update();
  }
  
  /**
   * Remove the channel from poller
   */
  void Remove() noexcept;  
 
  bool IsReading() const noexcept {
    return events_ & Event::ReadEvent;
  }

  bool IsWriting() const noexcept {
    return events_ & Event::WriteEvent;
  }
  
  bool IsNoneEvent() const noexcept {
    return events_ == Event::NoneEvent;
  }
  
  //!@} // events register
 
  //! \name events handler register
  //!@{
 
  void SetReadCallback(ReadEventCallback cb) { read_callback_ = std::move(cb); }
  void SetWriteCallback(EventCallback cb) { write_callback_ = std::move(cb); }
  void SetErrorCallback(EventCallback cb) { error_callback_ = std::move(cb); }
  void SetCloseCallback(EventCallback cb) { close_callback_ = std::move(cb); }

  //!@} // events handler resgiter
  
  //! \name getter
  //!@{
 
  //! Get the fd
  int GetFd() const noexcept { return fd_; }

  //! Get the events that fd interests
  int GetEvents() const noexcept { return events_; }
  
  //! Get the index
  int GetIndex() noexcept { return index_; }

  //!@} // getter
  
  //! \name setter
  //!@{
 
  void SetIndex(int index) noexcept { index_ = index; }
  void SetRevents(int event) noexcept { revents_ = event; }
  void SetLogHup(bool flag) noexcept { log_hup_ = flag; }

  //!@} // setter

  /**
   * Call the handler based on the occurred events
   *
   * This must be called by event loop
   */
  void HandleEvents(TimeStamp receive_time);

private:
  
  /**
   * Update the interested event in poller
   */
  void Update();
private:
  int fd_; //!< File descriptor that is being monitored
  int events_; //!< Events that fd interests
  int revents_; //!< Received(Occurred) events
  
  /**
   * Used for poller
   *
   * - To Poller, this is the index in the pollfds
   *   (To implemetate saerch and remove pollfd in 
   *    array in O(1))
   * - To Epoller, this is state of fd(Reuse index)
   */
  int index_;

  ReadEventCallback read_callback_;
  EventCallback write_callback_;
  EventCallback close_callback_;
  EventCallback error_callback_;

  /**
   * For logging message when connection is hupping
   */
  bool log_hup_;

  /**
   * For assert
   *
   * events_handing_ must be false when dtor is called
   * This force TcpConnection::RemoveConnection to call 
   */
  bool events_handling_;
  
  /** 
   * A channel must be tied with a event loop to
   * ensure the hanler must be called in one loop.
   *
   * So, the all data structure in the channel
   * no need to lock in multithread environments.
   */
  EventLoop* loop_;
};

//!@}

} // namespace kanon

#endif // KANON_CHANNEL_H
