#include "kanon/net/inet_addr.h"

#include <cctype>
#include <memory>
#include <string.h>

#ifdef KANON_ON_UNIX
#  include <netdb.h>
#  include <sys/socket.h>
#endif

#include "kanon/string/string_view.h"
#include "kanon/string/string_view_util.h"
#include "kanon/util/macro.h"
#include "kanon/util/mem.h"

#include "kanon/net/endian_api.h"
#include "kanon/net/sock_api.h"

using namespace kanon;

// Must ensure memory layout be same, so as get same offset member in union be
// right
static_assert(offsetof(sockaddr_in, sin_family) ==
                  offsetof(sockaddr_in6, sin6_family),
              "the offset of family member in sockaddr_in and sockaddr_in6 "
              "should be same");
static_assert(
    offsetof(sockaddr_in, sin_port) == offsetof(sockaddr_in6, sin6_port),
    "the offset of port member in sockaddr_in and sockaddr_in6 should be same");

static_assert(sizeof(InetAddr) == sizeof(struct sockaddr_in6),
              "InetAddr size should equal to sockaddr_in6");

InetAddr::InetAddr(Port port, bool loopback, bool v6)
{
  if (v6) {
    addr6_.sin6_port = sock::ToNetworkByteOrder16(port);
    addr6_.sin6_addr = loopback ? in6addr_loopback : in6addr_any;
    addr6_.sin6_family = AF_INET6;
  } else {
    addr_.sin_port = sock::ToNetworkByteOrder16(port);
    addr_.sin_addr.s_addr =
        sock::ToNetworkByteOrder32(loopback ? INADDR_LOOPBACK : INADDR_ANY);
    addr_.sin_family = AF_INET;
  }
}

InetAddr::InetAddr(StringView ip, Port port)
{
  if (ip.find('.') != StringView::npos) {
    sock::FromIpPort(ip.data(), port, addr_);
  } else if (ip.find(':') != StringView::npos) {
    sock::FromIpPort(ip.data(), port, addr6_);
  } else {
    throw InetAddr("This is invalid ip argument");
  }
}

InetAddr::InetAddr(StringView addr)
{
#if 0
  // Address format:
  // host-part:port-part

  // Domain name don't allowed to hold colon character(':')
  // So, there is no ambiguous

  auto last_colon_pos = addr.rfind(':');
  if (last_colon_pos == StringView::npos)
    throw InetAddrException("This a invalid address(No colon separator)");

  auto port_sv = addr.substr(last_colon_pos + 1);
  auto host = addr.substr_range(0, last_colon_pos);
  auto port = StringViewTo32(port_sv);

  if (!port || *port < 0 || *port >= (1 << 16))
    throw InetAddrException("Contains a invalid port number");

  if (host.find(':') != StringView::npos) {
    // Ipv6 address
    // Only Ipv6 address can hold colon in the host part
    sock::FromIpPort(host.ToString(), *port, addr6_);
  } else {
    bool is_ip4 = true;

    // The domain name and ipv4 both can hold dot('.') and digit.
    // To distinguish them, I separate the four decimal
    // If all match, that is ipv4
    // (NOTICE: the last part of domain name must not be digit)

    size_t dot_pos = -1;
    size_t next_dot_pos = 0;
    optional<int32_t> decimal = 0;
    for (int i = 0; i < 4; ++i) {
      next_dot_pos = addr.find('.', dot_pos + 1);
      if (next_dot_pos == StringView::npos) {
        if (i == 3) {
          decimal =
              StringViewTo32(addr.substr_range(dot_pos + 1, last_colon_pos));
          if (!(!decimal || *decimal < 0 || *decimal > 255)) break;
        }
        is_ip4 = false;
        break;
      }
      decimal = StringViewTo32(addr.substr_range(dot_pos + 1, next_dot_pos));
      if (!decimal || *decimal < 0 || *decimal > 255) {
        is_ip4 = false;
        break;
      }
      dot_pos = next_dot_pos;
    }

    if (is_ip4)
      sock::FromIpPort(host.ToString(), *port, addr_);
    else {
      auto const hostname = host.ToString();
      auto const service = port_sv.ToString();
      new (this) InetAddr(hostname, service);
    }
  }
#else
  if (std::isalpha(addr[0])) {
    // Hostname
    auto colon_pos = addr.find(':');
    if (colon_pos != StringView::npos) {
      auto const hostname = addr.substr(0, colon_pos).ToString();
      auto const service = addr.substr(colon_pos + 1).ToString();
      *this = InetAddr(hostname, service);
      return;
    }
  } else if ((std::isalnum(addr[0]) && addr.find('.') != StringView::npos) ||
             addr[0] == '*')
  {
    // Ipv4 address
    auto colon_pos = addr.find(':');
    if (colon_pos != StringView::npos) {
      auto ip = addr.substr(0, colon_pos).ToString();
      // ip must be dotted decimal presentation
      if (ip == "*") ip = "0.0.0.0";
      auto const port = addr.substr(colon_pos + 1).ToString();
      auto o_port = StringViewToU16(port);
      if (o_port) {
        sock::FromIpPort(ip, *o_port, addr_);
        return;
      }
    }
  } else if (addr[0] == '[') {
    auto right_pos = addr.find(']');
    if (right_pos != StringView::npos) {
      auto const ip = addr.substr_range(1, right_pos).ToString();
      auto const port = addr.substr(right_pos + 2).ToString();
      auto o_port = StringViewToU16(port);
      if (o_port) {
        sock::FromIpPort(ip, *o_port, addr6_);
        return;
      }
    }
  }

  throw InetAddrException("This a invalid address");
#endif
}

InetAddr::InetAddr(StringArg hostname, StringArg service)
{
  const auto addrs = Resolve(hostname, service, false);

  if (addrs.empty())
    throw InetAddrException("There is no matched hostname or service");

  *this = std::move(addrs.front());
}

InetAddr::Port InetAddr::GetPort() const KANON_NOEXCEPT
{
  return sock::ToHostByteOrder16(static_cast<Port>(addr_.sin_port));
}

std::string InetAddr::ToIpPort() const
{
  char buf[64];
  auto sa = IsIpv4() ? sock::to_sockaddr(&addr_) : sock::to_sockaddr(&addr6_);
  sock::ToIpPort(buf, sizeof buf, sa);

  return buf;
}

std::string InetAddr::ToIp() const
{
  char buf[64];
  auto sa = IsIpv4() ? sock::to_sockaddr(&addr_) : sock::to_sockaddr(&addr6_);
  sock::ToIp(buf, sizeof buf, sa);

  return buf;
}

std::vector<InetAddr> InetAddr::Resolve(StringArg hostname, StringArg service,
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
  // (Only useful when .ai_family is specified as AF_INET6)
  // If there are no IPv6 address could be found,
  // return Ipv4-mapped IPv6 address in the list instead.
  //
  // AI_ADDRCONFIG:
  // return ipv4 address only when ipv4 address is configured in local system
  // so does ipv6, this is useful on IPv4 only system since no IPv6 address
  // return then connect(2) and bind(2) don't fail infinitely.

  // AI_NUMERICSERV:
  // service must be a decimal port number instead service name
  hint.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG);

  if (is_server) {
    // AI_PASSIVE is suitable for binding socket,
    // otherwise, these returned address will be
    // suiltable for connect
    hint.ai_flags |= AI_PASSIVE;
  }

  return Resolve(hostname, service, &hint);
}

std::vector<InetAddr> InetAddr::Resolve(StringArg hostname, StringArg service,
                                        struct addrinfo const *hint)
{
  std::vector<InetAddr> addrs;

  struct addrinfo *addr_list;
  auto ret = ::getaddrinfo(hostname.data(), service.data(), hint, &addr_list);

  if (!ret) {
    // In this way, ensure free resource don't care which branch.
    // Also, it ensure exception-safety when exception is used.
    std::unique_ptr<struct addrinfo, void (*)(struct addrinfo *)> wrapped_addr(
        addr_list, &freeaddrinfo);
    KANON_UNUSED(wrapped_addr);

    for (; addr_list; addr_list = addr_list->ai_next) {
      if (addr_list->ai_family == AF_INET) {
        assert(addr_list->ai_addrlen == sizeof(struct sockaddr_in));
        addrs.emplace_back(
            *sock::sockaddr_cast<struct sockaddr_in>(addr_list->ai_addr));
      } else if (addr_list->ai_family == AF_INET6) {
        assert(addr_list->ai_addrlen == sizeof(struct sockaddr_in6));
        addrs.emplace_back(
            *sock::sockaddr_cast<struct sockaddr_in6>(addr_list->ai_addr));
      }
    }
  } else {
    LOG_SYSERROR_KANON << "Resolve the hostname to address error: "
                 << ::gai_strerror(ret);
    LOG_SYSERROR_KANON << "User can retry get address later";
  }

  return addrs;
}
