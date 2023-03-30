#ifndef KANON_NET_POLL_POLLER_H
#define KANON_NET_POLL_POLLER_H

#include <poll.h>
#include <unordered_map>

#include "kanon/net/poll/poller_base.h"

namespace kanon {

/**
 * \ingroup net
 * \addtogroup demultiplexer
 * @{
 */

/**
 * \brief Demultiplexer (poll(2) wrapper)
 *
 * As the role of Demultiplexer in Reactor design pattern
 *
 * \note
 * Support LT mode only
 *
 * \warning Internal class
 */
class KANON_NET_NO_API Poller final : public PollerBase {
 public:
  //! Construct a Poller
  explicit Poller(EventLoop *loop);
  ~Poller() KANON_NOEXCEPT override;

  TimeStamp Poll(int ms,
                 ChannelVec &active_channels) KANON_NOEXCEPT KANON_OVERRIDE;
  void UpdateChannel(Channel *ch) KANON_OVERRIDE;
  void RemoveChannel(Channel *ch) KANON_OVERRIDE;

 private:
  /*
   * struct pollfd {
   *  int fd;
   *  short events;
   *  short revents;
   * };
   */

  /**
   * poll() is different from epoll().
   *
   * It don't give such API like ::epoll_ctl()
   * we must register and remove in the pollfd_ to
   * control the interested events and fds
   *
   * Therefore it also don't split events and revents,
   * so need two member to indicates interested events
   * and occurred events.
   */
  std::vector<struct pollfd> pollfds_; //!< Array contains events

  /**
   * pollfd don't give such data member in epoll_data,
   * we need to maintain a map from fd to channel
   */
  std::unordered_map<int, Channel *> channels_map_; //!< Map: fd->channel
};

//!@}

} // namespace kanon

#endif // KANON_NET_POLL_POLLER_H
