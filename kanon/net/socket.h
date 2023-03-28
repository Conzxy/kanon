#ifndef KANON_NET_SOCKET_H
#define KANON_NET_SOCKET_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"

#include "kanon/net/type.h"
#include "kanon/net/sock_api.h"

namespace kanon {

class InetAddr;

//! \addtogroup net
//!@{

/**
 * \brief Represents socket instance
 * \note
 *   We don't create socket, just store the fd of socket
 *   and use the RAII property to avoid handler(fd) leak
 *
 *   Also, this class wrap some API in sock_api.h
 *   that make it easy to use.
 *
 *   Internal class
 */
class Socket : noncopyable {
 public:
  explicit Socket(FdType fd)
    : fd_{fd}
  {
  }

  ~Socket() noexcept;

  // Must be called by server
  void BindAddress(InetAddr const &addr) noexcept;
  int Accpet(InetAddr &addr) noexcept;

  void ShutdownWrite() noexcept;
  void ShutdownTwoDirection() noexcept;

  void SetReuseAddr(bool flag) noexcept { sock::SetReuseAddr(fd_, flag); }
  void SetReusePort(bool flag) noexcept { sock::SetReusePort(fd_, flag); }
  void SetNoDelay(bool flag) noexcept { sock::SetNoDelay(fd_, flag); }
  void SetKeepAlive(bool flag) noexcept { sock::SetKeepAlive(fd_, flag); }

  // Must be called by client

  int GetFd() const noexcept { return fd_; }

 private:
  FdType fd_;
};

//!@}

} // namespace kanon

#endif // KANON_NET_SOCKET_H
