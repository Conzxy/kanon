#include "sock_api.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <linux/version.h>
#include <netinet/tcp.h>

using namespace kanon;

void
sock::toIp(char* buf, size_t size, SA const* addr) {
	if (addr->sa_family == AF_INET) {
		assert(size >= INET_ADDRSTRLEN);
		auto addr4 = sockaddr_cast<sockaddr_in const>(addr);
		::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
	} else {
		assert(size >= INET6_ADDRSTRLEN);
		auto addr6 = sockaddr_cast<sockaddr_in6 const>(addr);
		::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
	}
}

void
sock::toIpPort(char* buf, size_t size, SA const* addr) {
	buf[0] = '[';
	toIp(buf+1, size, addr);
	auto len = ::strlen(buf);
	auto addr4 = sockaddr_cast<sockaddr_in const>(addr);
	::snprintf(buf+len, size-len, ";%hu]", addr4->sin_port);
}

void
setNonBlockAndCloExec(int fd) KANON_NOEXCEPT {
	auto ret = ::fcntl(fd, F_GETFL, 0);
	if (ret < 0)
		LOG_SYSFATAL << "fail to get file status flag(nonblock)";

	ret |= O_NONBLOCK;
	ret = ::fcntl(fd, F_SETFL, ret);
	if (ret < 0)
		LOG_SYSFATAL << "fail to set file status flag(nonblock)";
	
	ret = ::fcntl(fd, F_GETFD);
	if (ret < 0)
		LOG_SYSERROR << "fail to get file descriptor flag(cloexec)";

	ret |= O_CLOEXEC;
	ret = ::fcntl(fd, F_SETFD, ret);
	if (ret < 0)
		LOG_SYSERROR << "fail to set file descriptor flag(cloexec)";
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
#define NO_SOCKTYPE // SOCK_NONBLOCK, SOCK_CLOEXEC
#endif

int
sock::createNonBlockAndCloExec(bool ipv6) KANON_NOEXCEPT {
	int sockfd;
#ifdef NO_SOCKTYPE
	sockfd = ::socket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0) goto error_handle;
	setNonBlockAndCloExec(sockfd);
#else
	sockfd = ::socket(ipv6 ? AF_INET6 : AF_INET, 
					  SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
					  IPPROTO_TCP);
	if (sockfd < 0) goto error_handle;
#endif
	
	return sockfd;

error_handle:
	LOG_SYSFATAL << "create new socket fd error";

	return -1;
	
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28) 
#define NO_ACCEPT4
#endif

int
sock::accept(int fd, sockaddr_in6* addr) KANON_NOEXCEPT {
	socklen_t socklen = sizeof(struct sockaddr_in6);
					
	int cli_sock;
#ifdef NO_ACCEPT4
	cli_sock = ::accept(fd, sock::to_sockaddr(addr), &socklen);
	setNonBlockAndCloExec(cli_sock);
#else
	cli_sock = ::accept4(fd, sock::to_sockaddr(addr), &socklen, O_NONBLOCK | O_CLOEXEC);
#endif

	if (cli_sock < 0) {
		switch(errno) {
		case EAGAIN: // In linux, EAGAIN = EWOULDBLOCK
		case ECONNABORTED:
		case EINTR:
		case EMFILE: // per-process limit on the number of open fd has been reached
			LOG_SYSERROR << "accept() expected error occurred";
			break;
		case EFAULT:
		case EINVAL:
		case ENFILE:
		case ENOTSOCK:
		case EOPNOTSUPP:
		case EPROTO:
		case ENOBUFS:
		case ENOMEM:
			LOG_SYSFATAL << "accept() unexpected error occurred";
			break;
		default:
			LOG_SYSFATAL << "accept() unknown error occurred";
			break;
		}
	}

	return cli_sock;
}

void
sock::setReuseAddr(int fd, int flag) KANON_NOEXCEPT {
	auto ret = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, static_cast<socklen_t>(sizeof flag));

	if (ret < 0)
		LOG_SYSERROR << "setsockopt error(socket: reuse address)";
}


void
sock::setReusePort(int fd, int flag) KANON_NOEXCEPT {
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
	auto ret = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, static_cast<socklen_t>(sizeof flag));

	if (ret < 0)
		LOG_SYSERROR << "setsockopt error(socket: reuse port)";
#else
	LOG_INFO << "linux version less than 3.9, there no reuseport option can set";
#endif
}

void
sock::setNoDelay(int fd, int flag) KANON_NOEXCEPT {
	auto ret = ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, static_cast<socklen_t>(sizeof flag));

	if (ret < 0)
		LOG_SYSERROR << "set sockopt error(tcp: nodelay)";

}

int
sock::getsocketError(int fd) KANON_NOEXCEPT {
	int optval;
	auto len = static_cast<socklen_t>(sizeof optval);

	if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &len)) {
		return errno;
	} else {
		return optval;
	}
}
struct sockaddr_in6
sock::getLocalAddr(int fd) KANON_NOEXCEPT {
	struct sockaddr_in6 addr;
	socklen_t len = sizeof addr;
	::memset(&addr, 0, sizeof addr);

	if (::getsockname(fd, sock::to_sockaddr(&addr), &len)) {
		LOG_SYSERROR << "fail to get local address by getsockname";
	}

	return addr;
}

struct sockaddr_in6
sock::getPeerAddr(int fd) KANON_NOEXCEPT {
	struct sockaddr_in6 addr;
	socklen_t len = sizeof addr;
	::memset(&addr, 0, sizeof addr);

	if (::getpeername(fd, sock::to_sockaddr(&addr), &len)) {
		LOG_SYSERROR << "fail to get local address by getsockname";
	}

	return addr;
}
