#include "kanon/net/sock_api.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <linux/version.h>
#include <netinet/tcp.h>

#include "kanon/net/endian_api.h"

using namespace kanon;

void
sock::ToIp(char* buf, size_t size, sockaddr const* addr) {
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
sock::ToIpPort(char* buf, size_t size, sockaddr const* addr) {
  buf[0] = '[';
  ToIp(buf+1, size, addr);
  auto len = ::strlen(buf);
  auto addr4 = sockaddr_cast<sockaddr_in const>(addr);
  ::snprintf(buf+len, size-len, ";%hu]", sock::ToHostByteOrder16(addr4->sin_port));
}

void
sock::SetNonBlockAndCloExec(int fd) noexcept {
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
sock::CreateSocket(bool ipv6) noexcept {
  int sockfd = ::socket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0) {
    LOG_SYSFATAL << "create new socket fd error";
    return -1;
  }

  return sockfd;
}

int
sock::CreateNonBlockAndCloExecSocket(bool ipv6) noexcept {
  int sockfd;
#ifdef NO_SOCKTYPE
  sockfd = ::socket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0) goto error_handle;
  SetNonBlockAndCloExec(sockfd);
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
sock::Accept(int fd, sockaddr_in6* addr) noexcept {
  socklen_t socklen = sizeof(struct sockaddr_in6);
          
  int cli_sock;
#ifdef NO_ACCEPT4
  cli_sock = ::accept(fd, sock::to_sockaddr(addr), &socklen);
  SetNonBlockAndCloExec(cli_sock);
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
sock::SetReuseAddr(int fd, int flag) noexcept {
  auto ret = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, static_cast<socklen_t>(sizeof flag));

  if (ret < 0) {
    LOG_SYSERROR << "setsockopt error(SO_REUSEADDR)";
  }
  else {
    LOG_INFO << "SO_REUSEADDR option is set to " << static_cast<bool>(flag);
  }
}


void
sock::SetReusePort(int fd, int flag) noexcept {
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
  auto ret = ::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &flag, static_cast<socklen_t>(sizeof flag));

  if (ret < 0) {
    LOG_SYSERROR << "setsockopt error(SO_REUSEPORT)";
  }
  else {
    LOG_INFO << "SO_REUSEPORT option is set to " << static_cast<bool>(flag);
  }
#else
  LOG_INFO << "linux version less than 3.9, there no reuseport option can set";
#endif
}

void
sock::SetNoDelay(int fd, int flag) noexcept {
  auto ret = ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, static_cast<socklen_t>(sizeof flag));

  if (ret < 0)
    LOG_SYSERROR << "set sockopt error(tcp: nodelay)";

}

void
sock::SetKeepAlive(int fd, int flag) noexcept {
  auto ret = ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flag, static_cast<socklen_t>(sizeof flag));

  if (ret < 0) {
    LOG_SYSERROR << "set sockeopt error(socket: keepalive)";
  }
}

int
sock::GetSocketError(int fd) noexcept {
  int optval;
  auto len = static_cast<socklen_t>(sizeof optval);

  if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &len)) {
    return errno;
  } else {
    return optval;
  }
}

struct sockaddr_in6
sock::GetLocalAddr(int fd) noexcept {
  struct sockaddr_in6 addr;
  socklen_t len = sizeof addr;
  ::memset(&addr, 0, sizeof addr);

  if (::getsockname(fd, sock::to_sockaddr(&addr), &len)) {
    LOG_SYSERROR << "fail to get local address by getsockname";
  }

  return addr;
}

struct sockaddr_in6
sock::GetPeerAddr(int fd) noexcept {
  struct sockaddr_in6 addr;
  socklen_t len = sizeof addr;
  ::memset(&addr, 0, sizeof addr);

  if (::getpeername(fd, sock::to_sockaddr(&addr), &len)) {
    LOG_SYSERROR << "fail to get local address by getsockname";
  }

  return addr;
}

bool
sock::IsSelfConnect(int sockfd) noexcept {
  const auto local = sock::GetLocalAddr(sockfd);
  const auto peer = GetPeerAddr(sockfd);

  if (local.sin6_family == AF_INET) {
    auto local4 = sockaddr_cast<struct sockaddr_in const>(&local);
    auto peer4 = sockaddr_cast<struct sockaddr_in const>(&peer);

    return local4->sin_port == peer4->sin_port && 
      local4->sin_addr.s_addr == peer4->sin_addr.s_addr;
  } else {
    // sin6_addr not a simple 32bit unsigned integer
    return local.sin6_port == peer.sin6_port &&
      !memcmp(&local.sin6_addr, &peer.sin6_addr, sizeof(local.sin6_addr));
  }
}
