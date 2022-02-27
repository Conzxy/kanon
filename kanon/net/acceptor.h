#ifndef KANON_NET_ACCEPT_H
#define KANON_NET_ACCEPT_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"

#include "socket.h"
#include "channel.h"
#include "inet_addr.h"

#include <functional>

namespace kanon {

class EventLoop;

/**
 * @note
 * Internal class
 * Only used by TcpServer
 */
class Acceptor : noncopyable {
public:
  using NewConnectionCallback = std::function<void(int cli_fd, InetAddr const& cli_addr)>;

  Acceptor(EventLoop* loop, InetAddr const& addr, bool reuseport=false);
  ~Acceptor() noexcept;
  
  bool Listening() const noexcept
  { return listening_; }

  void Listen() noexcept;  

  void SetNewConnectionCallback(NewConnectionCallback cb) noexcept
  { new_connection_callback_ = std::move(cb); }
private:
  EventLoop* loop_;
  Socket socket_; // accept socket
  Channel channel_; // accept channel
  
  bool listening_;  
  int dummyfd_; // avoid busy loop 

  NewConnectionCallback new_connection_callback_;  
};

} // namespace kanon

#endif // KANON_NET_ACCEPTOR_H
