#ifndef KANON_NET_TCPSERVER_H
#define KANON_NET_TCPSERVER_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/string/string-view.h"
#include "type.h"
#include "kanon/util/type.h"
#include "kanon/net/callback.h"
#include "kanon/net/Acceptor.h"

#include <string>

namespace kanon {

class InetAddr;

class TcpServer : noncopyable {
public:
	TcpServer(EventLoop* loop,
			  InetAddr const& listen_addr,
			  StringArg name,
			  bool reuseport);
	
	~TcpServer() KANON_NOEXCEPT;	
	
	// set callback
	void setConnectionCallback(ConnectionCallback cb) KANON_NOEXCEPT
	{ connection_callback_ = std::move(cb); }

	void setMessageCallback(MessageCallback cb) KANON_NOEXCEPT
	{ message_callback_ = std::move(cb); }

	void setWriteCompleteCallback(WriteCompleteCallback cb) KANON_NOEXCEPT
	{ write_complete_callback_ = std::move(cb); }
private:
	typedef kanon::map<std::string, kanon::TcpConnectionPtr> ConnectionMap;
	
	// Must be called 	
	void newConnection(int cli_sock, InetAddr const& cli_addr);

	EventLoop* loop_;
	std::string const ip_port_;
	std::string const name_;
	
	Acceptor acceptor_;	

	ConnectionCallback connection_callback_;
	MessageCallback message_callback_;
	WriteCompleteCallback write_complete_callback_;
	
	uint32_t next_conn_id;
	ConnectionMap connections_;

}; 

} // namespace kanon

#endif // KANON_NET_TCPSERVER_H
