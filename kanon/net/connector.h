#ifndef KANON_NET_CONNECTOR_H
#define KANON_NET_CONNECTOR_H

#include <atomic>
#include <functional>

#include "kanon/net/timer/timer_id.h"
#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"
#include "kanon/util/optional.h"
#include "kanon/util/ptr.h"

#include "kanon/net/inet_addr.h"

namespace kanon {

class EventLoop;
class Channel;

//! \ingroup net
//! \addtogroup client
//! \brief Client parts
//!@{

/**
 * \brief Connect to peer(server)
 *
 * Support:
 * - Start to connect
 * - Stop connecting
 * - Restart to conenct
 *
 * \note
 *  Internal class(Used by TcpClient)
 *
 * \warning
 *  Connector no such API to disconnect peer which is managed by the client,
 *  otherwise the handler of disconnect will be coupled to the server.
 */
class Connector
  : public std::enable_shared_from_this<Connector>
  , noncopyable {
  enum State : uint8_t {
    kConnecting,
    kConnected,
    kDisconnected,
  };

  /* Don't restrict to tcp connection,
     user can pack sockfd to other structure
     which is also based connection */
  typedef std::function<void(int sockfd)> NewConnectionCallback;

 protected:
  Connector(EventLoop *loop, InetAddr const &serv_addr);
 public:

  static std::shared_ptr<Connector> NewConnector(EventLoop *loop,
                                                 InetAddr const &serv_addr)
  {
    return kanon::MakeSharedFromProtected<Connector>(loop, serv_addr);
  }

  ~Connector() noexcept;

  /**
   * \brief Start connect to peer
   *
   * Call Connect() to connect server in server address that passed
   * to the constructor. \n
   * If Connect() fails, continue call this until connect to peer successfully
   * \note Not thread safe, but in loop
   */
  void StartRun();

  /**
   * \brief Stop(Disable) connector
   *
   * If retry timer is setted, cancel it.
   * If connector in connecting state, remove the channel.
   *
   * To next call of the StartRun() don't work.
   * i.e. The connector is down after call.
   */
  void Stop();

  /**
   * \brief Restart connector to connect to peer
   *
   * Used in close callback when closed by peer passively
   * \note Not thread safe, but in loop
   */
  void Restrat();

  void SetNewConnectionCallback(NewConnectionCallback cb) noexcept
  {
    new_connection_callback_ = std::move(cb);
  }

  InetAddr const &GetServerAddr() const noexcept { return serv_addr_; }

 private:
  void SetState(State s) noexcept { state_ = s; }

  void Connect();

  /**
   * Continue process connect() progress
   * Only when errno is:
   * - EINPROGRESS
   * - EINTR
   * - EAGAIN
   */
  void CompleteConnect(int sockfd);

  /**
   * Retry to connect peer
   * Only when errno is:
   * - EAGAIN
   * - EHOSTUNREACH/ENETUNREACH
   * - ECONNREFUSED
   * - ETIMEDOUT
   * - ENOTAVAIL
   *
   * \param sockfd For closeing
   */
  void Retry(int sockfd);

  /**
   * Disable events and remove channel
   * \warning
   *  Must be called in loop
   */
  void ResetChannel();

  void StopInLoop();

  EventLoop *loop_;
  InetAddr serv_addr_;
  uint32_t retry_interval_; //!< Time interval of retry connect

  std::unique_ptr<Channel> channel_;

  NewConnectionCallback new_connection_callback_;

  /*
   * The connect_ and state_ are necessary.
   * e.g. Stop() need remove channel and close fd
   * when connector in the connecting state
   */

  State state_; //!< Current stage, used for checking

  /**
   * Disable connector or not
   */
  bool connect_;

  kanon::optional<TimerId> timer_; //!< Retry timer
};

//!@}

} // namespace kanon

#endif // KANON_NET_CONNECTOR_H
