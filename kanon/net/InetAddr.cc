#include "InetAddr.h"
#include "sock_api.h"

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
