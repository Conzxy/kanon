#ifndef KANON_SOCK_API_H
#define KANON_SOCK_API_H

#include "kanon/util/macro.h"
#include "kanon/log/logger.h"
#include "kanon/net/endian_api.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <type_traits>

namespace kanon {
namespace sock {

typedef struct sockaddr SA;

namespace detail {
// FIXME: or better check method exists?
template<typename T>
struct is_sockaddr : std::false_type {};

template<>
struct is_sockaddr<struct sockaddr_in> : std::true_type {};

template<>
struct is_sockaddr<struct sockaddr_in6> : std::true_type {};

template<>
struct is_sockaddr<SA> : std::true_type {};

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
KANON_CONSTEXPR if_const_add_const_t<S, SA>* to_sockaddr(S* addr) noexcept
{ return reinterpret_cast<if_const_add_const_t<S, SA>*>(addr); }


void toIpPort(char* buf, size_t size, SA const* addr);

void toIp(char* buf, size_t size, SA const* addr);

inline void fromIpPort(StringArg ip, 
             uint16_t port,
             sockaddr_in& addr) noexcept {
  addr.sin_family = AF_INET;
  addr.sin_port = toNetworkByteOrder16(port);

  if (!::inet_pton(addr.sin_family, ip, &addr.sin_addr)) {
    LOG_SYSFATAL << "fail to convert ip presentation to numeric";
  }
  
}

inline void fromIpPort(StringArg ip,
             uint16_t port,
             sockaddr_in6& addr) noexcept {
  addr.sin6_family = AF_INET6;
  addr.sin6_port = toNetworkByteOrder16(port);

  if (!::inet_pton(addr.sin6_family, ip, &addr.sin6_addr)) {
    LOG_SYSFATAL << "fail to convert ip presentation to numeric(ipv6)";
  }
}

void setNonBlockAndCloExec(int fd) noexcept;

int createSocket(bool ipv6) noexcept;
int createNonBlockAndCloExecSocket(bool ipv6) noexcept;

inline void close(int fd) noexcept {
  if (::close(fd) < 0) {
    LOG_SYSFATAL << "close socket error";
  }
}

inline void bind(int fd, SA const* addr) noexcept {
  auto ret = ::bind(fd, addr, 
            addr->sa_family == AF_INET ? 
            sizeof(struct sockaddr_in) :
            sizeof(struct sockaddr_in6));

  if (ret < 0) {
    LOG_SYSFATAL << "bind error";
  }
}

inline void listen(int fd) noexcept {
  auto ret = ::listen(fd, SOMAXCONN);

  if (ret < 0) {
    LOG_SYSFATAL << "listen error";
  }
}

inline int Connect(int fd, SA const* addr) noexcept {
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
int accept(int fd, struct sockaddr_in6* addr) noexcept;

// shutdown write direction of a specified connection
inline int ShutdownWrite(int fd) noexcept {
  return ::shutdown(fd, SHUT_WR);
}

// socket option
void setReuseAddr(int fd, int flag) noexcept;

void setReusePort(int fd, int flag) noexcept;

void SetNoDelay(int fd, int flag) noexcept;

void SetKeepAlive(int fd, int flag) noexcept;

int getsocketError(int fd) noexcept;



// get local and peer address
struct sockaddr_in6 getLocalAddr(int fd) noexcept;
struct sockaddr_in6 getPeerAddr(int fd) noexcept;


// io
ssize_t inline write(int fd, void const* data, size_t len) noexcept {
  return ::write(fd, data, len);
}

ssize_t inline read(int fd, void* data, size_t len) noexcept {
  return ::read(fd, data, len);
}

// check if self-connection
bool isSelfConnect(int sockfd) noexcept;

} // namespace sock
} // namespace sock

#endif // KANON_SOCK_API_H
