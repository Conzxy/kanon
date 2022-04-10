#include "kanon/net/inet_addr.h"

#include <string.h>
#include <netdb.h>
#include <memory>
#include <sys/socket.h>
#include <cctype>

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

InetAddr::InetAddr(StringView ip, Port port) {
  if (ip.find('.') != StringView::npos) {
    sock::FromIpPort(ip.data(), port, addr_);
  } 
  else if (ip.find(':') != StringView::npos) {
    sock::FromIpPort(ip.data(), port, addr6_);
  } 
  else {
    LOG_ERROR << "This is invalid ip argument:" << ip;
  }
}

InetAddr::InetAddr(StringView addr)
{      
  if (std::isalpha(addr[0])) {
    // Hostname
    auto colon_pos = addr.find(':');
    if (colon_pos != StringView::npos) {
      auto const hostname = addr.substr(0, colon_pos).ToString();
      auto const service = addr.substr(colon_pos+1).ToString();
      *this = InetAddr(hostname, service);
    }
  }
  else if (std::isalnum(addr[0]) && addr.find('.') != StringView::npos) {
    // Ipv4 address
    auto colon_pos = addr.find(':');
    if (colon_pos != StringView::npos) {
      auto const ip = addr.substr(0, colon_pos).ToString();
      auto const port = addr.substr(colon_pos+1).ToString();
      sock::FromIpPort(ip, ::atoi(port.c_str()), addr_);
    }
  }
  else if (addr[0] == '[') {
    // Ipv6 address
    auto right_pos = addr.find(']');

    if (right_pos != StringView::npos) {
      auto const ip = addr.substr_range(1, right_pos).ToString();
      auto const port = addr.substr(right_pos+2).ToString();
      sock::FromIpPort(ip.c_str(), ::atoi(port.c_str()), addr6_);
    }
  }
  else {
    LOG_ERROR << "This is invalid address string";
  }
}

InetAddr::InetAddr(StringArg hostname, StringArg service) {
  const auto addrs = Resolve(hostname, service);

  *this = std::move(addrs.front());
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
  bool is_server)
{
  struct addrinfo hint;
  BZERO(&hint, sizeof hint);

  // Support Ipv4 and Ipv6
  hint.ai_family = AF_UNSPEC;
  // Support TCP connection only
  hint.ai_socktype = SOCK_STREAM;

  // Looks up ipv4 and ipv6 address
  // AI_V4MAPPED:
  // 

  // AI_ADDRCONFIG:
  // control return ipv4 address 
  // only when ipv4 address is configured in local system
  // so does ipv6, this is useful on IPv4 only system.

  // AI_NUMERICSERV:
  // return decimal port number instead service name
  hint.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV);

 if (is_server) {
      // AI_PASSIVE is suitable for binding socket,
      // otherwise, these returned address will be 
      // suiltable for connect
      hint.ai_flags |= AI_PASSIVE;
  }
  
  return Resolve(hostname, service, &hint);  
}

std::vector<InetAddr> 
InetAddr::Resolve(
  StringArg hostname, 
  StringArg service,
  struct addrinfo const* hint)
{
  std::vector<InetAddr> addrs;
  
  struct addrinfo* addr_list;
  auto ret = ::getaddrinfo(hostname.data(), service.data(), hint, &addr_list);

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
    LOG_SYSERROR << "Resolve the hostname to address error: " << ::gai_strerror(ret);
    LOG_SYSERROR << "User can retry get address later";
  }

  return addrs;
}