#ifndef KANON_SOCK_API_H
#define KANON_SOCK_API_H

#include "kanon/util/macro.h"

#ifdef KANON_ON_UNIX
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#elif defined(KANON_ON_WIN)
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#endif

#include <fcntl.h>
#include <type_traits>

#include "kanon/util/arithmetic_type.h"
#include "kanon/log/logger.h"

#include "kanon/net/type.h"
#include "kanon/net/endian_api.h"

namespace kanon {

struct CompletionContext;

namespace sock {

typedef struct sockaddr sockaddr;

//! \cond
namespace detail {

template <typename T>
struct is_sockaddr : std::false_type {};

template <>
struct is_sockaddr<struct sockaddr_in> : std::true_type {};

template <>
struct is_sockaddr<struct sockaddr_in6> : std::true_type {};

template <>
struct is_sockaddr<sockaddr> : std::true_type {};

} // namespace detail

template <typename T>
struct is_sockaddr : detail::is_sockaddr<typename std::remove_cv<T>::type> {};

template <typename S, typename T,
          typename = typename std::enable_if<is_sockaddr<S>::value &&
                                             is_sockaddr<T>::value>::type>
KANON_CONSTEXPR S *sockaddr_cast(T *addr) KANON_NOEXCEPT
{
  return reinterpret_cast<S *>(addr);
}

template <typename T, typename U>
struct if_const_add_const
  : std::conditional<std::is_const<T>::value, typename std::add_const<U>::type,
                     U> {};

template <typename T, typename U>
using if_const_add_const_t = typename if_const_add_const<T, U>::type;

template <typename S,
          typename = typename std::enable_if<is_sockaddr<S>::value>::type>
KANON_CONSTEXPR if_const_add_const_t<S, sockaddr> *to_sockaddr(S *addr) KANON_NOEXCEPT
{
  return reinterpret_cast<if_const_add_const_t<S, sockaddr> *>(addr);
}

KANON_NET_API void ToIpPort(char *buf, size_t size, sockaddr const *addr);
KANON_NET_API void ToIp(char *buf, size_t size, sockaddr const *addr);

KANON_INLINE void FromIpPort(StringArg ip, uint16_t port,
                             sockaddr_in &addr) KANON_NOEXCEPT
{
  addr.sin_family = AF_INET;
  addr.sin_port = sock::ToNetworkByteOrder16(port);

  if (!::inet_pton(addr.sin_family, ip, &addr.sin_addr)) {
    LOG_SYSFATAL << "fail to convert ip presentation to numeric";
  }
}

KANON_INLINE void FromIpPort(StringArg ip, uint16_t port,
                             sockaddr_in6 &addr) KANON_NOEXCEPT
{
  addr.sin6_family = AF_INET6;
  addr.sin6_port = sock::ToNetworkByteOrder16(port);

  if (!::inet_pton(addr.sin6_family, ip, &addr.sin6_addr)) {
    LOG_SYSFATAL << "fail to convert ip presentation to numeric(ipv6)";
  }
}

KANON_NET_API void SetNonBlockAndCloExec(FdType fd) KANON_NOEXCEPT;

KANON_NET_API FdType CreateSocket(bool ipv6) KANON_NOEXCEPT;
KANON_NET_API FdType CreateNonBlockAndCloExecSocket(bool ipv6) KANON_NOEXCEPT;
KANON_NET_API FdType CreateOverlappedSocket(bool ipv6) KANON_NOEXCEPT;

KANON_INLINE void Close(FdType fd) KANON_NOEXCEPT
{
#ifdef KANON_ON_WIN
  int ret = ::closesocket(fd);
#elif defined(KANON_ON_UNIX)
  int ret = ::close(fd);
#endif
  if (ret < 0) {
    LOG_SYSFATAL << "close socket error";
  } else {
    LOG_DEBUG_KANON << "close fd = " << fd;
  }
}

KANON_INLINE void Bind(FdType fd, sockaddr const *addr) KANON_NOEXCEPT
{
  auto ret = ::bind(fd, addr,
                    addr->sa_family == AF_INET ? sizeof(struct sockaddr_in)
                                               : sizeof(struct sockaddr_in6));

  if (ret < 0) {
    LOG_SYSFATAL << "bind error";
  }
}

KANON_INLINE int Connect(FdType fd, sockaddr const *addr) KANON_NOEXCEPT
{
  auto ret =
      ::connect(fd, addr,
                addr->sa_family == AF_INET ? sizeof(struct sockaddr_in)
                                           : sizeof(struct sockaddr_in6));

  return ret;
}

#ifdef KANON_ON_WIN
KANON_NET_API bool WinConnect(FdType fd, sockaddr const *addr,
                          CompletionContext *ctx) KANON_NOEXCEPT;
#endif

KANON_NET_API void Listen(FdType fd) KANON_NOEXCEPT;

// Here, use sockaddr_in6 is better since we don't know the actual address of
// client. sockaddr_in6 cover the sockaddr If accept fill the ipv6 address,
// don't cause problem, ipv4 also ok. we don't need to distinguish two input
// parameter(more easier). According the family field in sockaddr, we can
// distinguish ipv6 and ipv4 then cast to correct address.
KANON_NET_API FdType Accept(FdType fd, struct sockaddr_in6 *addr) KANON_NOEXCEPT;

// shutdown write direction of a specified connection
KANON_INLINE int ShutdownWrite(FdType fd) KANON_NOEXCEPT
{
  return ::shutdown(fd, SHUT_WRITE);
}

KANON_INLINE int ShutdownTwoDirection(FdType fd) KANON_NOEXCEPT
{
  return ::shutdown(fd, SHUT_BOTH);
}

// socket option
KANON_NET_API void SetReuseAddr(FdType fd, int flag) KANON_NOEXCEPT;
KANON_NET_API void SetReusePort(FdType fd, int flag) KANON_NOEXCEPT;
KANON_NET_API void SetNoDelay(FdType fd, int flag) KANON_NOEXCEPT;
KANON_NET_API void SetKeepAlive(FdType fd, int flag) KANON_NOEXCEPT;
KANON_NET_API int GetSocketError(FdType fd) KANON_NOEXCEPT;

// get local and peer address
KANON_NET_API struct sockaddr_in6 GetLocalAddr(FdType fd) KANON_NOEXCEPT;
KANON_NET_API struct sockaddr_in6 GetPeerAddr(FdType fd) KANON_NOEXCEPT;

// io
KANON_INLINE isize Write(FdType fd, void const *data, size_t len) KANON_NOEXCEPT
{
#ifdef KANON_ON_UNIX
  return ::write(fd, data, len);
#endif
  return -1;
}

KANON_INLINE isize Read(FdType fd, void *data, size_t len) KANON_NOEXCEPT
{
#ifdef KANON_ON_UNIX
  return ::read(fd, data, len);
#endif
  return -1;
}

// check if self-connection
KANON_NET_API bool IsSelfConnect(FdType sockfd) KANON_NOEXCEPT;

} // namespace sock
} // namespace kanon

//! \endcond
//
#endif // KANON_SOCK_API_H
