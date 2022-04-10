#ifndef KANON_POLLER_H
#define KANON_POLLER_H

#include <vector>

#include "kanon/util/noncopyable.h"
#include "kanon/util/time_stamp.h"
#include "kanon/util/type.h"
#include "kanon/util/macro.h"
#include "kanon/net/channel.h"
#include "kanon/net/event_loop.h"

namespace kanon {

class EventLoop;

/**
 * \ingroup net
 * \addtogroup demultiplexer
 * \brief As the role of the Synchronous Event Demultiplexer in Reactor pattern
 *
 *  Get the ready fd which can read or write, etc.
 * @{
 */

/**
 * \brief Abstract base class of Poller and Epoller
 * \warning Internal class
 */
class PollerBase : noncopyable {
public:
  using ChannelVec = std::vector<Channel*>;

  explicit PollerBase(EventLoop* loop)
    : loop_{ loop }
  { }
  
  virtual ~PollerBase() noexcept = default;

  /**
   * \brief Get the channels that are ready
   * \param[in] ms Time duration of poll timeout in millisecond
   * \param[out] active_channels Empty array used for filling ready channles
   * \return
   *   A timestamp indicates when events occurred
   */
  virtual TimeStamp Poll(int ms, ChannelVec& active_channels) = 0;

  
  /**
   * \brief Update interested event of a channel
   * \param ch Channel that will be updated
   */
  virtual void UpdateChannel(Channel* ch) = 0;

  /**
   * \brief Remove channel from the interested table
   *
   * - To Poller, we just remove pollfd object from array
   * - To Epoller, we remove this from kernel events table
   *
   * \param ch Channel that will be removed
   */
  virtual void RemoveChannel(Channel* ch) = 0;

protected:
  //! Used by derived class to ensure "One loop per thread"
  void AssertInThread() noexcept {
    loop_->AssertInThread();
  }

private:
  /* 
   * Since AssertInThread is exposed to derived class
   * There no need to expose this to them
   */
  EventLoop* loop_;
};

//!@}
} // namespace kanon

#endif // KANON_POLLER_H
