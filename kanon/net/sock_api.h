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
KANON_CONSTEXPR S *sockaddr_cast(T *addr) noexcept
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
KANON_CONSTEXPR if_const_add_const_t<S, sockaddr> *to_sockaddr(S *addr) noexcept
{
  return reinterpret_cast<if_const_add_const_t<S, sockaddr> *>(addr);
}

void ToIpPort(char *buf, size_t size, sockaddr const *addr);

void ToIp(char *buf, size_t size, sockaddr const *addr);

KANON_INLINE void FromIpPort(StringArg ip, uint16_t port,
                             sockaddr_in &addr) noexcept
{
  addr.sin_family = AF_INET;
  addr.sin_port = sock::ToNetworkByteOrder16(port);

  if (!::inet_pton(addr.sin_family, ip, &addr.sin_addr)) {
    LOG_SYSFATAL << "fail to convert ip presentation to numeric";
  }
}

KANON_INLINE void FromIpPort(StringArg ip, uint16_t port,
                             sockaddr_in6 &addr) noexcept
{
  addr.sin6_family = AF_INET6;
  addr.sin6_port = sock::ToNetworkByteOrder16(port);

  if (!::inet_pton(addr.sin6_family, ip, &addr.sin6_addr)) {
    LOG_SYSFATAL << "fail to convert ip presentation to numeric(ipv6)";
  }
}

void SetNonBlockAndCloExec(FdType fd) noexcept;

FdType CreateSocket(bool ipv6) noexcept;
FdType CreateNonBlockAndCloExecSocket(bool ipv6) noexcept;
FdType CreateOverlappedSocket(bool ipv6) noexcept;

KANON_INLINE void Close(FdType fd) noexcept
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

KANON_INLINE void Bind(FdType fd, sockaddr const *addr) noexcept
{
  auto ret = ::bind(fd, addr,
                    addr->sa_family == AF_INET ? sizeof(struct sockaddr_in)
                                               : sizeof(struct sockaddr_in6));

  if (ret < 0) {
    LOG_SYSFATAL << "bind error";
  }
}

inline int Connect(FdType fd, sockaddr const *addr) noexcept
{
  auto ret =
      ::connect(fd, addr,
                addr->sa_family == AF_INET ? sizeof(struct sockaddr_in)
                                           : sizeof(struct sockaddr_in6));

  return ret;
}

#ifdef KANON_ON_WIN
bool WinConnect(FdType fd, sockaddr const *addr,
                CompletionContext *ctx) noexcept;
#endif

void Listen(FdType fd) noexcept;

// Here, use sockaddr_in6 is better since we don't know the actual address of
// client. sockaddr_in6 cover the sockaddr If accept fill the ipv6 address,
// don't cause problem, ipv4 also ok. we don't need to distinguish two input
// parameter(more easier). According the family field in sockaddr, we can
// distinguish ipv6 and ipv4 then cast to correct address.
FdType Accept(FdType fd, struct sockaddr_in6 *addr) noexcept;

// shutdown write direction of a specified connection
KANON_INLINE int ShutdownWrite(FdType fd) noexcept
{
  return ::shutdown(fd, SHUT_WRITE);
}

KANON_INLINE int ShutdownTwoDirection(FdType fd) noexcept
{
  return ::shutdown(fd, SHUT_BOTH);
}

// socket option
void SetReuseAddr(FdType fd, int flag) noexcept;

void SetReusePort(FdType fd, int flag) noexcept;

void SetNoDelay(FdType fd, int flag) noexcept;

void SetKeepAlive(FdType fd, int flag) noexcept;

int GetSocketError(FdType fd) noexcept;

// get local and peer address
struct sockaddr_in6 GetLocalAddr(FdType fd) noexcept;
struct sockaddr_in6 GetPeerAddr(FdType fd) noexcept;

// io
KANON_INLINE isize Write(FdType fd, void const *data, size_t len) noexcept
{
#ifdef KANON_ON_UNIX
  return ::write(fd, data, len);
#endif
  return -1;
}

KANON_INLINE isize Read(FdType fd, void *data, size_t len) noexcept
{
#ifdef KANON_ON_UNIX
  return ::read(fd, data, len);
#endif
  return -1;
}

// check if self-connection
bool IsSelfConnect(FdType sockfd) noexcept;

} // namespace sock
} // namespace kanon

//! \endcond
//
#endif // KANON_SOCK_API_H
