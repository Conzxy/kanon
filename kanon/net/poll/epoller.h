#ifndef KANON_NET_EPOLLER_H
#define KANON_NET_EPOLLER_H

#include <sys/epoll.h>

#include "kanon/util/time_stamp.h"

#include "kanon/net/poll/poller_base.h"

namespace kanon {

/**
 * \ingroup net
 * \addtogroup demultiplexer
 * @{
 */

/**
 * \brief Demultiplexer (epoll(2) wrapper)
 *
 * As the role of Demultiplexer in reactor design pattern
 * \note 
 * Support LT OR ET mode
 *
 * \warning Internal class
 */
class Epoller final : public PollerBase {
public:
  //! Construct Epoller
  explicit Epoller(EventLoop* loop);
  ~Epoller() noexcept override;

  TimeStamp Poll(int ms, ChannelVec& active_channels) KANON_OVERRIDE;
  
  void UpdateChannel(Channel* ch) KANON_OVERRIDE;
  void RemoveChannel(Channel* ch) KANON_OVERRIDE;

  /**
   * \brief Set Epoller working in edge trigger mode
   * 
   * In fact, epolle() cannot be set to edge trigger mode
   * Set this will add EPOLL_ET to interested events of 
   * fd
   */
  void SetEdgeTriggertMode() noexcept
  { is_et_mode_ = true; }
  
  //! Check if working in edge trigger mode
  bool IsEdgeTriggerMode() const noexcept
  { return is_et_mode_; }

private:
  //! Helper of Poll()
  void FillActiveChannels(int ev_nums, 
                          ChannelVec& activeChannels) noexcept;

  //! Helper of UpdateChannel()
  void UpdateEpollEvent(int op, Channel* ch) noexcept;

private:
  // typedef union epoll_data {
  //  void *ptr;
  //  int fd;
  //  uint32_t u32;
  //  uint64_t u64;
  // } epoll_data_t;
  //
  // struct epoll_event {
  //   uint32_t events; // Epoll events
  //   epoll_data_t data; // user data variable
  // };
  using Event = struct epoll_event;
  
  int epoll_fd_; //!< fd of epoller()

  /**
   * The events in the epoll_event as two role:
   *   - The interested events 
   *   - The occurred events
   *
   * Besides, it can fill a epoll_data member data
   * we can put Channel* to epoll_data.ptr, \n
   * then we no need to maintain a data structure 
   * to manage Channels
   *
   * This used in ::epoll_wait() to get ready events.\n
   * Now, the events is "revents" and data.ptr is the
   * tied channel
   */
  std::vector<Event> events_; //<! array contains ready events 

  bool is_et_mode_ = false; //<! Whether is working in ET mode
};

//!@}

} // namespace kanon

#endif // KANON_NET_EPOLLER_H
