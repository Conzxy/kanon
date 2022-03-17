#ifndef KANON_NET_SOCKET_H
#define KANON_NET_SOCKET_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"

#include "kanon/net/sock_api.h"

namespace kanon {

class InetAddr;

/**
 * We don't create socket, just store the fd
 * and use the RAII property to avoid handler(fd) leak
 * 
 * Also, this class wrap some API in @file sock_api.h
 * that make it easy to use.
 * @note Internal class
 */
class Socket : noncopyable {
public:
  explicit Socket(int fd)
    : fd_{ fd }
  { }
  
  ~Socket() noexcept;  

  // Must be called by server  
  void bindAddress(InetAddr const& addr) noexcept;
  int accept(InetAddr& addr) noexcept;

  void ShutdownWrite() noexcept;

  void SetReuseAddr(bool flag) noexcept
  { sock::SetReuseAddr(fd_, flag); }
  void SetReusePort(bool flag) noexcept
  { sock::SetReusePort(fd_, flag); }
  void SetNoDelay(bool flag) noexcept
  { sock::SetNoDelay(fd_, flag); }
  void SetKeepAlive(bool flag) noexcept
  { sock::SetKeepAlive(fd_, flag); }

  // Must be called by client
  
  int GetFd() const noexcept
  { return fd_; }
private:
  int fd_;
};

} // namespace kanon

#endif // KANON_NET_SOCKET_H
