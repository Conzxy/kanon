#ifndef KANON_NET_ACCEPT_H
#define KANON_NET_ACCEPT_H

#include <functional>
#include <atomic>

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"

#include "kanon/net/socket.h"
#include "kanon/net/channel.h"
#include "kanon/net/inet_addr.h"

namespace kanon {

class EventLoop;

//! \ingroup net
//! \addtogroup server
//! \brief Server parts
//!@{

/**
 * \brief Accept connection from client
 *
 * Precisely, this is a accept() wrapper
 * TcpServer resgister NewConnectionCallback to
 * create a TcpConnection instance
 * \note
 *   Internal class
 *   Only used by TcpServer
 */
class Acceptor : noncopyable {
public:
  using NewConnectionCallback = std::function<void(int cli_fd, InetAddr const& cli_addr)>;
  
  /**
   * \brief Construct a Acceptor in @p addr
   * \param addr Address that want to listen
   * \param reuseport Reuse the same port(Useful to multiprocess model)
   *
   * Set socket options and channel read callback
   * then call Socket::BindAddress() to bind \p address
   * and start monitoring read event(i.e. new connection)
   */
  Acceptor(EventLoop* loop, InetAddr const& addr, bool reuseport=false);
  ~Acceptor() noexcept;
  
  /**
   * \brief Check whether is listening state 
   * \note
   *  Thread-safe
   */
  bool Listening() const noexcept
  { return listening_; }
  
  /**
   * \brief start listening
   *
   * \note
   *   Not thread-safe
   *   Ensured by TcpServer
   */
  void Listen() noexcept;  

  void SetNewConnectionCallback(NewConnectionCallback cb) noexcept
  { new_connection_callback_ = std::move(cb); }
private:
  EventLoop* loop_; //!< Ensure "One loop per thread"
  Socket socket_; //!< Accept socket
  Channel channel_; //!< Accept channel
  
  std::atomic_bool listening_; //!< Whether start listening
  int dummyfd_; //!< Avoid busy loop 

  NewConnectionCallback new_connection_callback_;  
};

//!@}
//
} // namespace kanon

#endif // KANON_NET_ACCEPTOR_H
