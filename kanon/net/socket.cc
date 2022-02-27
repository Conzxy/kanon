#include "socket.h"

#include "kanon/log/logger.h"
#include "inet_addr.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h> // IPPROTO_TCP

using namespace kanon;

Socket::~Socket() noexcept {
  if (::close(fd_)) {
    LOG_SYSERROR << "close(" << fd_ << ") error occurred";
  }
}

void
Socket::bindAddress(InetAddr const& addr) noexcept {
  sock::bind(fd_, addr.IsIpv4() ? 
      sock::to_sockaddr(addr.ToIpv4()) : sock::to_sockaddr(addr.ToIpv6()));
}

int
Socket::accept(InetAddr& addr) noexcept {
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
Socket::ShutdownWrite() noexcept {
  if (sock::ShutdownWrite(fd_)) {
    LOG_SYSERROR << "shutdown write error";
  }
}
