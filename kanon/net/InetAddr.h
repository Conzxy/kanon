#ifndef KANON_INET_ADDR_H
#define KANON_INET_ADDR_H

#include "kanon/util/macro.h"
#include "kanon/string/string-view.h"

#include <vector>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>

/*
 * IPv4 socket address structure
 * struct sockaddr_in {
 *     sa_family_t		sin_family;
 *     in_port_t		sin_port;
 *	   struct in_addr	sin_addr;
 * };
 *
 * struct in_addr {
 *     in_addr_t s_addr;
 * };
 *
 * IPv6 socket address structure
 * struct sockaddr_in6 {
 *     sa_family_t		sin6_family;
 *     in_port_t		sin6_port;
 *     uint32_t			sin6_flowinfo;
 *     struct in6_addr	sin6_addr;
 *     uint32_t			sin6_scoped_id;
 * };
 *
 * struct in6_addr {
 *     unsigned char s6_addr[16];
 * };
 *
 * obviously, the offset of family and port field is same,
 * so in union, we use these field which no need to distinguish.
 */

/*
 * struct addrinfo {
 *   int				ai_flags; // Hints argument flags
 *   int				ai_family; // specified ip family(1st arg)
 *   int				ai_socktype; // specified socket type(2nd arg)
 *   int 				ai_protocol; // specified protocol(3rd arg)
 *   socklen_t			ai_addrlen; // address length
 *   struct	sockaddr*	ai_addr; // point to the sockaddr structure
 *   char*				ai_canonname; // official name
 *   struct addrinfo*	ai_next; // point to next item in linked list
 * };
 */
struct addrinfo;

namespace kanon {


class InetAddr {
public:
	typedef uint16_t Port;

	enum class HintType {
		kNoneHint,
		kServer,
		kClient,
	};

	// Must be used by server
	explicit InetAddr(Port port=0, bool loopback=false, bool v6=false);
	
	// normally, used by client
	explicit InetAddr(StringArg ip, Port port, bool v6=false);

	// compatible with C struct @c sockaddr_in and @c sockaddr_in6
	InetAddr(struct sockaddr_in addr)
		: addr_{ addr }
	{ }

	InetAddr(struct sockaddr_in6 addr6)
		: addr6_{ addr6 }
	{ }

	std::string toIpPort() const;
	std::string toIp() const;	

	// sin_family
	sa_family_t family() const KANON_NOEXCEPT
	{ return addr_.sin_family; }
	
	Port port() const KANON_NOEXCEPT
	{ return static_cast<Port>(addr_.sin_port); }	
	
	bool isIpv4() const KANON_NOEXCEPT
	{ return addr_.sin_family == AF_INET; }

	struct sockaddr_in const* toIpv4() const KANON_NOEXCEPT
	{ return addr_.sin_family == AF_INET ? &addr_ : nullptr; }

	struct sockaddr_in6 const* toIpv6() const KANON_NOEXCEPT
	{ return addr_.sin_family == AF_INET6 ? &addr6_ : nullptr; }
	
	// resolve hostname(no service)
	static void resolve(StringArg hostname, std::vector<InetAddr>& addrs,
						HintType type = HintType::kNoneHint);

	static void resolve(StringArg hostname, std::vector<InetAddr>& addrs,
						struct addrinfo* hint);
private:
	union {
		sockaddr_in addr_;
		sockaddr_in6 addr6_;
	};
};

}

#endif // KANON_INET_ADDR_H
