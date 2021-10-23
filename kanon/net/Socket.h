#ifndef KANON_NET_SOCKET_H
#define KANON_NET_SOCKET_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "sock_api.h"

namespace kanon {

class InetAddr;

class Socket : noncopyable {
public:
	explicit Socket(int fd)
		: fd_{ fd }
	{ }
	
	~Socket() KANON_NOEXCEPT;	

	// Must be called by server	
	void bindAddress(InetAddr const& addr) KANON_NOEXCEPT;
	int accept(InetAddr& addr) KANON_NOEXCEPT;
	void shutdownWrite() KANON_NOEXCEPT;

	void setReuseAddr(bool flag) KANON_NOEXCEPT
	{ sock::setReuseAddr(fd_, flag); }
	void setReusePort(bool flag) KANON_NOEXCEPT
	{ sock::setReusePort(fd_, flag); }
	void setNoDelay(bool flag) KANON_NOEXCEPT
	{ sock::setNoDelay(fd_, flag); }
	void setKeepAlive(bool flag) KANON_NOEXCEPT
	{ sock::setKeepAlive(fd_, flag); }

	// Must be called by client
	
	int fd() const KANON_NOEXCEPT
	{ return fd_; }
private:
	int fd_;
};

} // namespace kanon

#endif // KANON_NET_SOCKET_H
