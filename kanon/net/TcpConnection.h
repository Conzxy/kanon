#ifndef KANON_NET_TCPCONNECTION_H
#define KANON_NET_TCPCONNECTION_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/net/type.h"
#include "kanon/net/callback.h"
#include "kanon/net/InetAddr.h"
#include "kanon/util/unique_ptr.h"

namespace kanon {

class Socket;
class Channel;

class TcpConnection : noncopyable
					, std::enable_shared_from_this<TcpConnection> {
public:
	TcpConnection(EventLoop* loop,
				  std::string const& name,
				  int sockfd,
				  InetAddr const& local_addr,
				  InetAddr const& peer_addr);

	~TcpConnection() KANON_NOEXCEPT;
	
	// since server thread will dispatch connection to IO thread
	// so need run some function in loop to ensure thread safe	
	EventLoop* loop() const KANON_NOEXCEPT
	{ return loop_; }	

	// set callback
	void setMessageCallback(MessageCallback cb)
	{ message_callback_ = std::move(cb); }

	void setConnectionCallback(ConnectionCallback cb)
	{ connection_callback_ = std::move(cb); }

	void setWriteCompleteCallback(WriteCompleteCallback cb)
	{ write_complete_callback_ = std::move(cb); }
private:
	EventLoop* loop_;
	std::string const name_;
	// use pointer just don't want to expose them to user
	std::unique_ptr<Socket> socket_;
	std::unique_ptr<Channel> channel_;	
	InetAddr const local_addr_;
	InetAddr const peer_addr_;

	MessageCallback message_callback_;
	ConnectionCallback connection_callback_;
	WriteCompleteCallback write_complete_callback_;
};

} // namespace kanon

#endif // KANON_NET_TCPCONNECTION_H
