#ifndef KANON_NET_TCPCONNECTION_H
#define KANON_NET_TCPCONNECTION_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/net/callback.h"
#include "kanon/net/InetAddr.h"
#include "kanon/util/unique_ptr.h"
#include "kanon/net/Buffer.h"

namespace kanon {

class Socket;
class Channel;
class EventLoop;

class TcpConnection : noncopyable
					, public std::enable_shared_from_this<TcpConnection> {
	enum State {
		kConnecting = 0,
		kConnected,
		kDisconnecting,
		kDisconnected,
		STATE_NUM,
	};
	
	static char const* state_str_[STATE_NUM];
public:
	TcpConnection(EventLoop* loop,
				  std::string const& name,
				  int sockfd,
				  InetAddr const& local_addr,
				  InetAddr const& peer_addr);
	
	~TcpConnection() KANON_NOEXCEPT;
	
	// since server thread will dispatch connection to IO thread,
	// need run some function in loop to ensure thread safe	
	EventLoop* loop() const KANON_NOEXCEPT
	{ return loop_; }	

	// set callback
	void setMessageCallback(MessageCallback cb)
	{ message_callback_ = std::move(cb); }

	void setConnectionCallback(ConnectionCallback cb)
	{ connection_callback_ = std::move(cb); }

	void setWriteCompleteCallback(WriteCompleteCallback cb)
	{ write_complete_callback_ = std::move(cb); }
	
	// should be called by server
	void setCloseCallback(CloseCallback cb)
	{ close_callback_ = std::move(cb); }	
	
	void setHighWaterMarkCallback(HighWaterMarkCallback cb)
	{ high_water_mark_callback_ = std::move(cb); }

	// field infomation
	bool isConnected() const KANON_NOEXCEPT
	{ return state_ == kConnected; }

	char const* state2String() const KANON_NOEXCEPT
	{ return state_str_[state_]; }
	
	std::string const& name() const KANON_NOEXCEPT
	{ return name_; }

	InetAddr const& localAddr() const KANON_NOEXCEPT
	{ return local_addr_; }

	InetAddr const& peerAddr() const KANON_NOEXCEPT
	{ return peer_addr_; }
	
	// interface for user to consume(read)
	Buffer* inputBuffer() KANON_NOEXCEPT
	{ return &input_buffer_; }

	Buffer* outputBuffer() KANON_NOEXCEPT
	{ return &output_buffer_; }
	
	// when TcpServer accept a new connection in newConnectionCallback
	void connectionEstablished();
	// when TcpServer has removed connection from its connections_
	void connectionDestroyed();
private:
	void handleRead(TimeStamp receive_time);
	void handleWrite();
	void handleError();
	void handleClose();

	EventLoop* loop_;
	std::string const name_;
	// use pointer just don't want to expose them to user
	std::unique_ptr<Socket> socket_;
	std::unique_ptr<Channel> channel_;	
	InetAddr const local_addr_;
	InetAddr const peer_addr_;
	
	State state_;

	Buffer input_buffer_;
	Buffer output_buffer_;
	
	MessageCallback message_callback_;
	ConnectionCallback connection_callback_;
	WriteCompleteCallback write_complete_callback_;
	HighWaterMarkCallback high_water_mark_callback_;
	CloseCallback close_callback_; // only be called by server
};

} // namespace kanon

#endif // KANON_NET_TCPCONNECTION_H
