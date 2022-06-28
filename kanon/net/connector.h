#ifndef KANON_NET_CONNECTOR_H
#define KANON_NET_CONNECTOR_H

#include <functional>
#include <atomic>

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/ptr.h"
#include "kanon/util/optional.h"
#include "kanon/net/timer/timer_id.h"

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
 * \note
 *   Internal class(User by TcpClient)
 */
class Connector : public std::enable_shared_from_this<Connector>, noncopyable {
  enum State : uint8_t {
    kConnecting,
    kConnected,
    kDisconnected,
  };

  typedef std::function<void(int sockfd)> NewConnectionCallback;  

public:
  Connector(EventLoop* loop,
            InetAddr const& serv_addr);

  static std::shared_ptr<Connector> NewConnector(EventLoop* loop, InetAddr const& serv_addr)
  { return std::make_shared<Connector>(loop, serv_addr); }

  ~Connector() noexcept;

  /**
   * \brief Start connect to peer
   *
   * Call Connect() to connect server in server address that passed
   * to the constructor. \n
   * If Connect() fails, continue call this until connect to peer successfully
   * \note Not thread safe, but in loop
   */
  void StartRun() noexcept;

  /**
   * \brief Stop retry connect to peer when fails infinitely
   *
   * Only useful when connectino is not established successfully
   */
  void Stop() noexcept;

  /**
   * \brief Restart connector to connect to peer
   *
   * Used in close callback when closed by peer passively
   * \note Not thread safe, but in loop
   */
  void Restrat() noexcept;

  void SetNewConnectionCallback(NewConnectionCallback cb) noexcept
  { new_connection_callback_ = std::move(cb); }

  InetAddr const& GetServerAddr() const noexcept {
    return serv_addr_;
  }

private:
  void SetState(State s) noexcept { state_ = s; }

  void Connect() noexcept;
  void CompleteConnect(int sockfd) noexcept;
  void Retry(int sockfd) noexcept;
  int RemoveAndResetChannel() noexcept;

  void StopInLoop() noexcept;

  EventLoop* loop_;
  InetAddr serv_addr_;
  uint32_t retry_interval_;  //!< Time interval of retry connect

  std::unique_ptr<Channel> channel_;

  NewConnectionCallback new_connection_callback_;

  /** 
   * The connect_ and state_ are necessary. \n
   * e.g. Stop() need remove channle and close fd
   * when connector in the connecting state
   */
  State state_; //!< Current stage, used for checking
  bool connect_; //!< Control whether continue connect to peer

  kanon::optional<TimerId> timer_; //!< Retry timer
};

//!@}

} // namespace kanon

#endif // KANON_NET_CONNECTOR_H
