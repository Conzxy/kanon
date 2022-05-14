#ifndef KANON_SOCK_API_H
#define KANON_SOCK_API_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <type_traits>

#include "kanon/util/macro.h"
#include "kanon/log/logger.h"

#include "kanon/net/endian_api.h"

namespace kanon {
namespace sock {

typedef struct sockaddr sockaddr;

//! \cond
namespace detail {

template<typename T>
struct is_sockaddr : std::false_type {};

template<>
struct is_sockaddr<struct sockaddr_in> : std::true_type {};

template<>
struct is_sockaddr<struct sockaddr_in6> : std::true_type {};

template<>
struct is_sockaddr<sockaddr> : std::true_type {};

} // namespace detail

template<typename T>
struct is_sockaddr : detail::is_sockaddr<typename std::remove_cv<T>::type> {};

template<typename S, typename T, typename = typename std::enable_if<
    is_sockaddr<S>::value && is_sockaddr<T>::value>::type>
KANON_CONSTEXPR S* sockaddr_cast(T* addr) noexcept
{ return reinterpret_cast<S*>(addr); }

template<typename T, typename U>
struct if_const_add_const 
  : std::conditional<std::is_const<T>::value,
    typename std::add_const<U>::type,
    U>
{ };

template<typename T, typename U>
using if_const_add_const_t = typename if_const_add_const<T, U>::type;

template<typename S, typename = typename std::enable_if<
    is_sockaddr<S>::value>::type>
KANON_CONSTEXPR if_const_add_const_t<S, sockaddr>* to_sockaddr(S* addr) noexcept
{ return reinterpret_cast<if_const_add_const_t<S, sockaddr>*>(addr); }

void ToIpPort(char* buf, size_t size, sockaddr const* addr);

void ToIp(char* buf, size_t size, sockaddr const* addr);

inline void FromIpPort(StringArg ip, 
             uint16_t port,
             sockaddr_in& addr) noexcept {
  addr.sin_family = AF_INET;
  addr.sin_port = sock::ToNetworkByteOrder16(port);

  if (!::inet_pton(addr.sin_family, ip, &addr.sin_addr)) {
    LOG_SYSFATAL << "fail to convert ip presentation to numeric";
  }
  
}

inline void FromIpPort(StringArg ip,
             uint16_t port,
             sockaddr_in6& addr) noexcept {
  addr.sin6_family = AF_INET6;
  addr.sin6_port = sock::ToNetworkByteOrder16(port);

  if (!::inet_pton(addr.sin6_family, ip, &addr.sin6_addr)) {
    LOG_SYSFATAL << "fail to convert ip presentation to numeric(ipv6)";
  }
}

void SetNonBlockAndCloExec(int fd) noexcept;

int CreateSocket(bool ipv6) noexcept;
int CreateNonBlockAndCloExecSocket(bool ipv6) noexcept;

inline void Close(int fd) noexcept {
  if (::close(fd) < 0) {
    LOG_SYSFATAL << "close socket error";
  } else {
    LOG_DEBUG_KANON << "close fd = " << fd;
  }
}

inline void Bind(int fd, sockaddr const* addr) noexcept {
  auto ret = ::bind(fd, addr, 
            addr->sa_family == AF_INET ? 
            sizeof(struct sockaddr_in) :
            sizeof(struct sockaddr_in6));

  if (ret < 0) {
    LOG_SYSFATAL << "bind error";
  }
}

inline void Listen(int fd) noexcept {
  auto ret = ::listen(fd, SOMAXCONN);

  if (ret < 0) {
    LOG_SYSFATAL << "listen error";
  }
}

inline int Connect(int fd, sockaddr const* addr) noexcept {
  auto ret = ::connect(fd, addr,
             addr->sa_family == AF_INET ?
             sizeof(struct sockaddr_in) :
             sizeof(struct sockaddr_in6));

  return ret;
}

// Here, use sockaddr_in6 is better since we don't know the actual address of client.
// sockaddr_in6 cover the sockaddr
// If accept fill the ipv6 address, don't cause problem, ipv4 also ok.
// we don't need to distinguish two input parameter(more easier).
// According the family field in sockaddr,
// we can distinguish ipv6 and ipv4 then cast to correct address.
int Accept(int fd, struct sockaddr_in6* addr) noexcept;

// shutdown write direction of a specified connection
inline int ShutdownWrite(int fd) noexcept {
  return ::shutdown(fd, SHUT_WR);
}

inline int ShutdownTwoDirection(int fd) noexcept {
  return ::shutdown(fd, SHUT_RDWR);
}

// socket option
void SetReuseAddr(int fd, int flag) noexcept;

void SetReusePort(int fd, int flag) noexcept;

void SetNoDelay(int fd, int flag) noexcept;

void SetKeepAlive(int fd, int flag) noexcept;

int GetSocketError(int fd) noexcept;

// get local and peer address
struct sockaddr_in6 GetLocalAddr(int fd) noexcept;
struct sockaddr_in6 GetPeerAddr(int fd) noexcept;

// io
ssize_t inline Write(int fd, void const* data, size_t len) noexcept {
  return ::write(fd, data, len);
}

ssize_t inline Read(int fd, void* data, size_t len) noexcept {
  return ::read(fd, data, len);
}

// check if self-connection
bool IsSelfConnect(int sockfd) noexcept;

} // namespace sock
} // namespace sock

//! \endcond
//
#endif // KANON_SOCK_API_H
