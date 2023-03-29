#include "kanon/net/sock_api.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace kanon;

void sock::Listen(FdType fd) noexcept
{
  auto ret = ::listen(fd, SOMAXCONN);

  if (ret < 0) {
    LOG_SYSFATAL << "listen error";
  }
}

FdType sock::CreateNonBlockAndCloExecSocket(bool ipv6) noexcept
{
  FdType sockfd;
#ifdef NO_SOCKTYPE
  sockfd = ::socket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0) goto error_handle;
  SetNonBlockAndCloExec(sockfd);
#else
  sockfd = ::socket(ipv6 ? AF_INET6 : AF_INET,
                    SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (sockfd < 0) goto error_handle;
#endif
  return sockfd;

error_handle:
  LOG_SYSFATAL << "create new socket fd error";

  return -1;
}

FdType sock::Accept(FdType fd, sockaddr_in6 *addr) noexcept
{
  socklen_t socklen = sizeof(struct sockaddr_in6);

#ifdef NO_ACCEPT4
  auto cli_sock = ::accept(fd, sock::to_sockaddr(addr), &socklen);
  SetNonBlockAndCloExec(cli_sock);
#else
  auto cli_sock =
      ::accept4(fd, sock::to_sockaddr(addr), &socklen, O_NONBLOCK | O_CLOEXEC);
#endif
  if (cli_sock < 0) {
    switch (errno) {
      case EAGAIN: // In linux, EAGAIN = EWOULDBLOCK
      case ECONNABORTED:
      case EINTR:
      case EMFILE: // per-process limit on the number of open fd has been
                   // reached
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

int sock::GetSocketError(FdType fd) noexcept
{
  int optval;
  auto len = static_cast<socklen_t>(sizeof optval);

  if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&optval, &len)) {
    return errno;
  } else {
    return optval;
  }
}
