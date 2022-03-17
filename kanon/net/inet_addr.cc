#include "kanon/net/inet_addr.h"

#include <string.h>
#include <netdb.h>
#include <memory>
#include <sys/socket.h>

#include "kanon/string/string_view.h"
#include "kanon/util/macro.h"
#include "kanon/util/mem.h"

#include "kanon/net/sock_api.h"
#include "kanon/net/endian_api.h"

using namespace kanon;

// make memory layout be same, so as get same offset member in union be right
static_assert(offsetof(sockaddr_in, sin_family) 
      == offsetof(sockaddr_in6, sin6_family),
      "the offset of family member in sockaddr_in and sockaddr_in6 should be same");
static_assert(offsetof(sockaddr_in, sin_port)
      == offsetof(sockaddr_in6, sin6_port),
      "the offset of port member in sockaddr_in and sockaddr_in6 should be same");

static_assert(sizeof(InetAddr) == sizeof(struct sockaddr_in6), 
      "InetAddr size should equal to sockaddr_in6");

static inline struct addrinfo MakeHint(const int flag, const int family) noexcept
{
  struct addrinfo hint;
  MemoryZero(&hint, sizeof hint);

  hint.ai_socktype = SOCK_STREAM;
  hint.ai_flags = flag;
  hint.ai_family = family;

  return hint;
}

InetAddr::InetAddr(Port port, bool loopback, bool v6) {
  if (v6) {
    addr6_.sin6_port = sock::ToNetworkByteOrder16(port);
    addr6_.sin6_addr = loopback ? in6addr_loopback : in6addr_any;
    addr6_.sin6_family = AF_INET6;
  } else {
    addr_.sin_port = sock::ToNetworkByteOrder16(port);
    addr_.sin_addr.s_addr = sock::ToNetworkByteOrder32(loopback ? INADDR_LOOPBACK : INADDR_ANY);
    addr_.sin_family = AF_INET;
  }
}

InetAddr::InetAddr(StringArg ip, Port port, bool v6) {
  if (v6) {
    sock::FromIpPort(ip, port, addr6_);
  } else {
    sock::FromIpPort(ip, port, addr_);
  }
}

InetAddr::InetAddr(StringArg hostname, StringArg service) {
  const auto addrs = Resolve(hostname, service, MakeHint(AI_ALL, AF_INET));

  // Bitwise is also ok
  *this = addrs.front();
}

InetAddr::Port InetAddr::GetPort() const noexcept
{
  return sock::ToHostByteOrder16(static_cast<Port>(addr_.sin_port));
}

std::string 
InetAddr::ToIpPort() const {
  char buf[64];
  auto sa = IsIpv4() ? sock::to_sockaddr(&addr_) : sock::to_sockaddr(&addr6_);  
  sock::ToIpPort(buf, sizeof buf, sa);

  return buf;
}

std::string
InetAddr::ToIp() const {
  char buf[64];
  auto sa = IsIpv4() ? sock::to_sockaddr(&addr_) : sock::to_sockaddr(&addr6_);
  sock::ToIp(buf, sizeof buf, sa);

  return buf;
}

std::vector<InetAddr>
InetAddr::Resolve(
  StringArg hostname, 
  StringArg service,
  HintType type) {
  struct addrinfo hint;
  BZERO(&hint, sizeof hint);

  switch (type) {
    case HintType::kClient:
      hint.ai_socktype = SOCK_STREAM;
      hint.ai_flags |= AI_NUMERICSERV | AI_ADDRCONFIG;
      break;
    case HintType::kServer:
      hint.ai_socktype = SOCK_STREAM;
      hint.ai_flags |= AI_NUMERICSERV | AI_PASSIVE | AI_ADDRCONFIG;
    break;
    case HintType::kNoneHint:
      hint.ai_socktype = SOCK_STREAM;
      hint.ai_flags |= AI_ALL;
    default:
      break;
  }
  
  return Resolve(hostname, service, hint);  
}

std::vector<InetAddr> 
InetAddr::Resolve(
  StringArg hostname, 
  StringArg service,
  struct addrinfo const& hint) {
  std::vector<InetAddr> addrs;
  
  struct addrinfo* addr_list;
  auto ret = ::getaddrinfo(hostname.data(), service.data(), &hint, &addr_list);

  auto addrinfo_deleter = 
    [](struct addrinfo* const info)
    {
      ::freeaddrinfo(info);
    };

  // In this way, ensure free resource don't care which branch.
  // Also, it ensure exception-safety when exception is used.
  std::unique_ptr<struct addrinfo, decltype(addrinfo_deleter)> wrapped_addr(
    addr_list, std::move(addrinfo_deleter)); KANON_UNUSED(wrapped_addr);

  if (!ret) {
    for (; addr_list; addr_list = addr_list->ai_next) {
      if (addr_list->ai_family == AF_INET) {
        assert(addr_list->ai_addrlen == sizeof(struct sockaddr_in));
        addrs.emplace_back(
            *sock::sockaddr_cast<struct sockaddr_in>(addr_list->ai_addr));
      }
      else if (addr_list->ai_family == AF_INET6) {
        assert(addr_list->ai_addrlen == sizeof(struct sockaddr_in6));
        addrs.emplace_back(
            *sock::sockaddr_cast<struct sockaddr_in6>(addr_list->ai_addr));
      }
    }
  } else {
    LOG_SYSERROR << "resolve the hostname to address error: " << ::gai_strerror(ret);
  }

  return addrs;
}