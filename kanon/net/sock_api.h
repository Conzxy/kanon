#ifndef KANON_SOCK_API_H
#define KANON_SOCK_API_H

#include "kanon/util/macro.h"
#include "kanon/log/Logger.h"
#include "kanon/net/endian_api.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <type_traits>

namespace kanon {
namespace sock {

typedef sockaddr SA;

namespace detail {
// FIXME: or better check method exists?
template<typename T>
struct is_sockaddr : std::false_type {};

template<>
struct is_sockaddr<sockaddr_in> : std::true_type {};

template<>
struct is_sockaddr<sockaddr_in6> : std::true_type {};

} // namespace detail

template<typename T>
struct is_sockaddr : detail::is_sockaddr<typename std::remove_cv<T>::type> {};

template<typename S, typename T, typename = typename std::enable_if<
		is_sockaddr<S>::value>::type>
KANON_CONSTEXPR S* sockaddr_cast(T* addr) KANON_NOEXCEPT
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
KANON_CONSTEXPR if_const_add_const_t<S, SA>* to_sockaddr(S* addr) KANON_NOEXCEPT
{ return reinterpret_cast<if_const_add_const_t<S, SA>*>(addr); }


void toIpPort(char* buf, size_t size, SA const* addr);

void toIp(char* buf, size_t size, SA const* addr);

inline void fromIpPort(StringArg ip, 
					   uint16_t port,
					   sockaddr_in& addr) KANON_NOEXCEPT {
	addr.sin_family = AF_INET;
	addr.sin_port = toNetworkByteOrder16(port);

	if (!::inet_pton(addr.sin_family, ip, &addr.sin_addr)) {
		LOG_SYSFATAL << "fail to convert ip presentation to numeric";
	}
	
}

inline void fromIpPort(StringArg ip,
					   uint16_t port,
					   sockaddr_in6& addr) KANON_NOEXCEPT {
	addr.sin6_family = AF_INET6;
	addr.sin6_port = toNetworkByteOrder16(port);

	if (!::inet_pton(addr.sin6_family, ip, &addr.sin6_addr)) {
		LOG_SYSFATAL << "fail to convert ip presentation to numeric(ipv6)";
	}
}

void setNonBlockAndCloExec(int fd) KANON_NOEXCEPT;

int createNonBlockAndCloExec(bool ipv6) KANON_NOEXCEPT;

inline void bind(int fd, SA const* addr) KANON_NOEXCEPT {
	auto ret = ::bind(fd, addr, 
					  addr->sa_family == AF_INET ? 
					  sizeof(struct sockaddr_in) :
					  sizeof(struct sockaddr_in6));

	if (ret < 0) {
		LOG_SYSFATAL << "bind error";
	}
}

inline void listen(int fd) KANON_NOEXCEPT {
	auto ret = ::listen(fd, SOMAXCONN);

	if (ret < 0) {
		LOG_SYSFATAL << "listen error";
	}
}

inline void connect(int fd, SA const* addr) KANON_NOEXCEPT {
	auto ret = ::connect(fd, addr,
						 addr->sa_family == AF_INET ?
						 sizeof(struct sockaddr_in) :
						 sizeof(struct sockaddr_in6));

	if (ret < 0) {
		LOG_SYSFATAL << "connect error";
	}
}

// use ipv6 is better
// if it is ipv4 in fact, just plain convert it to sockaddr_in by reinterpret_cast
int accept(int fd, struct sockaddr_in6* addr) KANON_NOEXCEPT;

// socket option
void setReuseAddr(int fd, int flag) KANON_NOEXCEPT;

void setReusePort(int fd, int flag) KANON_NOEXCEPT;

void setNoDelay(int fd, int flag) KANON_NOEXCEPT;

int getsocketError(int fd) KANON_NOEXCEPT;

// get local and peer address
struct sockaddr_in6 getLocalAddr(int fd) KANON_NOEXCEPT;
struct sockaddr_in6 getPeerAddr(int fd) KANON_NOEXCEPT;


// io
ssize_t inline write(int fd, void const* data, size_t len) KANON_NOEXCEPT {
	return ::write(fd, data, len);
}

ssize_t inline read(int fd, void* data, size_t len) KANON_NOEXCEPT {
	return ::read(fd, data, len);
}

} // namespace sock
} // namespace sock

#endif // KANON_SOCK_API_H
