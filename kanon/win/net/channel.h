#ifndef KANON_WIN_NET_CHANNEL_H__
#define KANON_WIN_NET_CHANNEL_H__

#include <functional>
#include <string>

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/time_stamp.h"
#include "kanon/log/logger.h"

#include "kanon/net/macro.h"
#include "kanon/net/event.h"
#include "kanon/net/type.h"

namespace kanon {

struct CompletionContext;
class ChunkList;
class EventLoop;
class Buffer;

/**
 * \ingroup net
 * \addtogroup dispatcher
 * \brief Event Dispatcher
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
 public:
  /**
   * \brief Construct a Channel
   * \param fd file descriptor that we want to monitor
   * \warning
   *   The fd is not managed by this.\n
   *   In kanon, this is maybe socket, timerfd, eventfd
   *   that can be polled(otherwise, will trigger POLLINVAL).\n
   *   i.e. The fd is not released in the dtor.
   */
  Channel(EventLoop *loop, FdType fd);
  ~Channel() noexcept;

  //! \name conversion
  //!@{

  //! Convert event to string representation
  static std::string Ev2String(uint32_t ev);

  //! Convert interested event to string representation
  KANON_INLINE std::string Events2String() const noexcept
  {
    return Ev2String(events_);
  }

  //! Convert receive event to string representation
  KANON_INLINE std::string Revents2String() const noexcept
  {
    return Ev2String(events_);
  }

  //!@} // conversion

  //! \name event register
  //!@{

  //! Start monitoring the read event
  void EnableReading() noexcept
  {
    events_ |= Event::ReadEvent;
    Update();
  }

  //! Start monitoring the write event
  void EnableWriting() noexcept
  {
    events_ |= Event::WriteEvent;
    Update();
  }

  //! Stop monitoring the read event
  void DisableReading() noexcept
  {
    events_ &= ~Event::ReadEvent;
    // Update();
  }

  //! Stop monitoring the write event
  void DisableWriting() noexcept
  {
    events_ &= ~Event::WriteEvent;
    // Update();
  }

  //! Stop monitoring any event
  void DisableAll() noexcept
  {
    events_ = 0;
    // Update();
  }

  /**
   * Remove the channel from poller
   */
  void Remove() noexcept;

  bool IsReading() const noexcept { return events_ & Event::ReadEvent; }
  bool IsWriting() const noexcept { return events_ & Event::WriteEvent; }
  bool IsNoneEvent() const noexcept { return events_ == 0; }

  //!@} // events register

  //! \name events handler register
  //!@{

  void SetReadCallback(ReadEventCallback cb) { read_callback_ = std::move(cb); }
  void SetWriteCallback(EventCallback cb) { write_callback_ = std::move(cb); }
  void SetErrorCallback(EventCallback cb) { error_callback_ = std::move(cb); }
  void SetCloseCallback(EventCallback cb) { close_callback_ = std::move(cb); }

  void RegisterCompletionContext(CompletionContext *ctx);

  //!@} // events handler resgiter

  //! \name getter
  //!@{

  //! Get the fd
  FdType GetFd() const noexcept { return fd_; }

  //! Get the events that fd interests

  int GetEvents() const noexcept { return events_; }

  // int GetRevents() const noexcept { return revents_; }

  //! Get the index
  int GetIndex() noexcept { return index_; }

  //!@} // getter

  //! \name setter
  //!@{

  void SetEvents(int ev) noexcept { events_ = ev; }
  void SetIndex(int index) noexcept { index_ = index; }
  // void SetRevents(int event) noexcept { revents_ = event; }

  //!@} // setter

  /**
   * Call the handler based on the occurred events
   *
   * This must be called by event loop
   */
  void HandleEvents(TimeStamp receive_time);

  KANON_INLINE void PushCompleteContext(CompletionContext *ctx)
  {
    completion_contexts_.push_back(ctx);
  }

 private:
  /**
   * Update the interested event in poller
   */
  void Update();

 private:
  FdType fd_; //!< File descriptor that is being monitored

  int events_; //!< Events that fd interests

  /**
   * Used for poller
   *
   * - To Poller, this is the index in the pollfds
   *   (To implemetate saerch and remove pollfd in
   *    array in O(1))
   * - To Epoller, this is state of fd(Reuse index)
   */
  int index_;

  /**
   * A channel must be tied with a event loop to
   * ensure the hanler must be called in one loop.
   *
   * So, the all data structure in the channel
   * no need to lock in multithread environments.
   */
  EventLoop *loop_;

  ReadEventCallback read_callback_;
  EventCallback write_callback_;
  EventCallback close_callback_;
  EventCallback error_callback_;

#ifndef NDEBUG
  /**
   * For assert
   *
   * events_handing_ must be false when dtor is called
   * This force TcpConnection::RemoveConnection to call
   */
  bool events_handling_;
#endif

  std::vector<CompletionContext *> completion_contexts_;

 public:
  size_t transferred_bytes = -1;
};
//!@}

} // namespace kanon
#endif