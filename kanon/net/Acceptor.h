#ifndef KANON_NET_ACCEPT_H
#define KANON_NET_ACCEPT_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "Socket.h"
#include "Channel.h"
#include "InetAddr.h"

#include <functional>

namespace kanon {

class EventLoop;

class Acceptor : noncopyable {
public:
	typedef std::function<void(int cli_fd, InetAddr const& cli_addr)> NewConnectionCallback;

	Acceptor(EventLoop* loop, InetAddr const& addr, bool reuseport=false);
	~Acceptor() KANON_NOEXCEPT;
	
	bool listening() const KANON_NOEXCEPT
	{ return listening_; }

	void listen() KANON_NOEXCEPT;	

	void setNewConnectionCallback(NewConnectionCallback cb) KANON_NOEXCEPT
	{ new_connection_callback_ = std::move(cb); }
private:
	EventLoop* loop_;
	Socket socket_; // accept socket
	Channel channel_; // accept channel
	
	bool listening_;	
	int dummyfd_; // avoid busy loop 

	NewConnectionCallback new_connection_callback_;	
};

} // namespace kanon

#endif // KANON_NET_ACCEPTOR_H
