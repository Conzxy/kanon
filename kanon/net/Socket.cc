#include "Socket.h"

#include "kanon/log/Logger.h"
#include "InetAddr.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h> // IPPROTO_TCP

using namespace kanon;

Socket::~Socket() KANON_NOEXCEPT {
	if (::close(fd_)) {
		LOG_SYSERROR << "close(" << fd_ << ") error occurred";
	}
}

void
Socket::bindAddress(InetAddr const& addr) KANON_NOEXCEPT {
	sock::bind(fd_, addr.isIpv4() ? 
			sock::to_sockaddr(addr.toIpv4()) : sock::to_sockaddr(addr.toIpv6()));
}

int
Socket::accept(InetAddr& addr) KANON_NOEXCEPT {
	struct sockaddr_in6 addr6;
	auto cli_fd = sock::accept(fd_, &addr6);
	
	if (cli_fd >= 0) {
		if (addr6.sin6_family == AF_INET) {
			new (&addr) InetAddr(*reinterpret_cast<sockaddr_in*>(&addr6));		
		} else {
			new (&addr) InetAddr(addr6);
		}
	}

	return cli_fd;
}

void
Socket::shutdownWrite() KANON_NOEXCEPT {
	if (sock::shutdownWrite(fd_)) {
		LOG_SYSERROR << "shutdown write error";
	}
}
