#include "kanon/net/socket.h"

#ifdef KANON_ON_UNIX
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h> // IPPROTO_TCP
#endif

#include "kanon/log/logger.h"
#include "kanon/net/inet_addr.h"

using namespace kanon;

Socket::~Socket() KANON_NOEXCEPT { sock::Close(fd_); }

void Socket::BindAddress(InetAddr const &addr) KANON_NOEXCEPT
{
  sock::Bind(fd_, addr.ToSockaddr());
}

int Socket::Accpet(InetAddr &addr) KANON_NOEXCEPT
{
  struct sockaddr_in6 addr6;
  auto cli_fd = sock::Accept(fd_, &addr6);

  if (cli_fd >= 0) {
    if (addr6.sin6_family == AF_INET) {
      new (&addr) InetAddr(*reinterpret_cast<sockaddr_in *>(&addr6));
    } else {
      new (&addr) InetAddr(addr6);
    }
  }

  return cli_fd;
}

void Socket::ShutdownWrite() KANON_NOEXCEPT
{
  LOG_TRACE_KANON << "Shutdown peer in write direction";

  if (sock::ShutdownWrite(fd_)) {
    LOG_SYSERROR << "Shutdown write error";
  }
}

void Socket::ShutdownTwoDirection() KANON_NOEXCEPT
{
  LOG_TRACE_KANON << "Shutdown peer in two dierction(read/write)";

  if (sock::ShutdownTwoDirection(fd_)) {
    LOG_SYSERROR << "Shutdown read/write error";
  }
}
