#include "kanon/net/sock_api.h"

#include <assert.h>

#ifdef KANON_ON_LINUX
#include <linux/version.h>
#endif

#ifdef KANON_ON_UNIX
#include <netinet/tcp.h>
#endif

#ifdef KANON_ON_WIN
#include <mswsock.h>
#endif

#include <stdio.h>
#include <string.h>

#include "kanon/net/endian_api.h"

using namespace kanon;

void sock::ToIp(char *buf, size_t size, sockaddr const *addr)
{
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

void sock::ToIpPort(char *buf, size_t size, sockaddr const *addr)
{
  if (addr->sa_family == AF_INET6) {
    buf[0] = '[';
    buf++;
    size--;
  }
  ToIp(buf, size, addr);
  auto len = ::strlen(buf);
  if (addr->sa_family == AF_INET6) buf[len++] = ']';
  auto addr4 = sockaddr_cast<sockaddr_in const>(addr);
  ::snprintf(buf + len, size - len, ":%hu",
             sock::ToHostByteOrder16(addr4->sin_port));
}

void sock::SetNonBlockAndCloExec(FdType fd) KANON_NOEXCEPT
{
#ifdef KANON_ON_UNIX
  auto ret = ::fcntl(fd, F_GETFL, 0);
  if (ret < 0) LOG_SYSFATAL << "fail to get file status flag";

  ret |= (O_NONBLOCK | O_CLOEXEC);
  ret = ::fcntl(fd, F_SETFL, ret);
  if (ret < 0)
    LOG_SYSFATAL << "fail to set file status flag(O_NONBLOCK | O_CLOEXEC)";
#endif
}

#ifdef KANON_ON_UNIX
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
#define NO_SOCKTYPE // SOCK_NONBLOCK, SOCK_CLOEXEC
#endif
#endif

FdType sock::CreateSocket(bool ipv6) KANON_NOEXCEPT
{
  int sockfd = ::socket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0) {
    LOG_SYSFATAL << "create new socket fd error";
    return -1;
  }

  return sockfd;
}

#ifdef KANON_ON_LINUX
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
#define NO_ACCEPT4
#endif
#endif

void sock::SetReuseAddr(FdType fd, int flag) KANON_NOEXCEPT
{
  auto ret = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char const *)&flag,
                          static_cast<socklen_t>(sizeof flag));
  if (ret < 0) {
    LOG_SYSERROR << "setsockopt error(SO_REUSEADDR)";
  } else {
    LOG_INFO << "SO_REUSEADDR option is set to " << static_cast<bool>(flag);
  }
}

void sock::SetReusePort(FdType fd, int flag) KANON_NOEXCEPT
{
#ifdef KANON_ON_LINUX
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
  auto ret = ::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &flag,
                          static_cast<socklen_t>(sizeof flag));

  if (ret < 0) {
    LOG_SYSERROR << "setsockopt error(SO_REUSEPORT)";
  } else {
    LOG_INFO << "SO_REUSEPORT option is set to " << static_cast<bool>(flag);
  }
#else
  LOG_INFO << "linux version less than 3.9, there no reuseport option can set";
#endif
#endif
}

void sock::SetNoDelay(FdType fd, int flag) KANON_NOEXCEPT
{
  auto ret = ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char const *)&flag,
                          static_cast<socklen_t>(sizeof flag));

  if (ret < 0) LOG_SYSERROR << "set sockopt error(tcp: nodelay)";
}

void sock::SetKeepAlive(FdType fd, int flag) KANON_NOEXCEPT
{
  auto ret = ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char const *)&flag,
                          static_cast<socklen_t>(sizeof flag));

  if (ret < 0) {
    LOG_SYSERROR << "set sockeopt error(socket: keepalive)";
  }
}

struct sockaddr_in6 sock::GetLocalAddr(FdType fd) KANON_NOEXCEPT
{
  struct sockaddr_in6 addr;
  socklen_t len = sizeof addr;
  ::memset(&addr, 0, sizeof addr);

  if (::getsockname(fd, sock::to_sockaddr(&addr), &len)) {
    LOG_SYSERROR << "fail to get local address by getsockname()";
  }

  return addr;
}

struct sockaddr_in6 sock::GetPeerAddr(FdType fd) KANON_NOEXCEPT
{
  struct sockaddr_in6 addr;
  socklen_t len = sizeof addr;
  ::memset(&addr, 0, sizeof addr);

  if (::getpeername(fd, sock::to_sockaddr(&addr), &len)) {
    LOG_SYSERROR << "fail to get peer address by getpeername()";
  }

  return addr;
}

static bool GetLocalAddrSafe(FdType fd, struct sockaddr_in6 &addr) KANON_NOEXCEPT
{
  socklen_t len = sizeof addr;
  ::memset(&addr, 0, sizeof addr);

  if (::getsockname(fd, sock::to_sockaddr(&addr), &len)) {
    LOG_SYSERROR << "fail to get peer address by getsockname()";
    return false;
  }
  return true;
}

static bool GetPeerAddrSafe(FdType fd, struct sockaddr_in6 &addr) KANON_NOEXCEPT
{
  socklen_t len = sizeof addr;
  ::memset(&addr, 0, sizeof addr);
  if (::getpeername(fd, sock::to_sockaddr(&addr), &len)) {
    LOG_SYSERROR << "fail to get peer address by getpeerkname()";
    return false;
  }
  return true;
}

bool sock::IsSelfConnect(FdType sockfd) KANON_NOEXCEPT
{
  struct sockaddr_in6 local;
  struct sockaddr_in6 peer;
  const auto local_ok = GetLocalAddrSafe(sockfd, local);
  const auto peer_ok = GetPeerAddrSafe(sockfd, peer);

  // Connection is not established ever
  // This is not a error of self-connect
  if (!local_ok || !peer_ok) return false;

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
