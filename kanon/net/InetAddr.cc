#include "InetAddr.h"
#include "sock_api.h"

#include <string.h>
#include <netdb.h>

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

InetAddr::InetAddr(Port port, bool loopback, bool v6) {
  if (v6) {
    addr6_.sin6_port = sock::toNetworkByteOrder16(port);
    addr6_.sin6_addr = loopback ? in6addr_loopback : in6addr_any;
    addr6_.sin6_family = AF_INET6;
  } else {
    addr_.sin_port = sock::toNetworkByteOrder16(port);
    addr_.sin_addr.s_addr = sock::toNetworkByteOrder32(loopback ? INADDR_ANY : INADDR_LOOPBACK);
    addr_.sin_family = AF_INET;
  }
}

InetAddr::InetAddr(StringArg ip, Port port, bool v6) {
  if (v6) {
    sock::fromIpPort(ip, port, addr6_);
  } else {
    sock::fromIpPort(ip, port, addr_);
  }
}

std::string 
InetAddr::toIpPort() const {
  char buf[64];
  auto sa = isIpv4() ? sock::to_sockaddr(&addr_) : sock::to_sockaddr(&addr6_);  
  sock::toIpPort(buf, sizeof buf, sa);

  return buf;
}

std::string
InetAddr::toIp() const {
  char buf[64];
  auto sa = isIpv4() ? sock::to_sockaddr(&addr_) : sock::to_sockaddr(&addr6_);
  sock::toIp(buf, sizeof buf, sa);

  return buf;
}

void
InetAddr::resolve(StringArg hostname, std::vector<InetAddr>& addrs,
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
    default:
      break;
  }
  
  resolve(hostname, addrs, &hint);  
}

void
InetAddr::resolve(StringArg hostname, std::vector<InetAddr>& addrs,
          struct addrinfo* hint) {
  assert(addrs.size() == 0);
  
  struct addrinfo* addr_list;
  auto ret = ::getaddrinfo(hostname, NULL, hint, &addr_list);

  if (!ret) {
    for (; addr_list; addr_list = addr_list->ai_next) {
      if (addr_list->ai_family == AF_INET)
        addrs.emplace_back(
            *sock::sockaddr_cast<struct sockaddr_in>(addr_list->ai_addr));
      else
        addrs.emplace_back(
            *sock::sockaddr_cast<struct sockaddr_in6>(addr_list->ai_addr));
    }

    ::freeaddrinfo(addr_list);
  } else {
    LOG_SYSERROR << "resolve the hostname to address error: " << ::gai_strerror(ret);
  }
}

