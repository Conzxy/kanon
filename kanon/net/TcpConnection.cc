#include "kanon/net/TcpConnection.h"
#include "kanon/net/Socket.h"
#include "kanon/net/Channel.h"
#include "kanon/net/EventLoop.h"

using namespace kanon;

char const* 
TcpConnection::state_str_[STATE_NUM] = {
	"connecting",
	"connected",
	"disconnecting",
	"disconnected"
};

TcpConnection::TcpConnection(EventLoop*  loop,
							 std::string const& name,
							 int sockfd,
							 InetAddr const& local_addr,
							 InetAddr const& peer_addr)
	: loop_{ loop }
	, name_{ name }
	, socket_{ kanon::make_unique<Socket>(sockfd) }
	, channel_{ kanon::make_unique<Channel>(loop, sockfd) }
	, local_addr_{ local_addr }
	, peer_addr_{ peer_addr }
	, state_{ kConnecting }
{
	channel_->setReadCallback([this](TimeStamp receive_time) {
		this->handleRead(receive_time);
	});

	channel_->setWriteCallback([this]() {
		this->handleWrite();
	});

	channel_->setErrorCallback([this]() {
		this->handleError();
	});
	
	channel_->setCloseCallback([this]() {
		this->handleClose();
	});

	// FIXME: keepalive
	//
	LOG_TRACE << "TcpConnection::ctor [" << name_ << "] created";
	
}

TcpConnection::~TcpConnection() KANON_NOEXCEPT {
	assert(state_ == kDisconnected);
	LOG_TRACE << "TcpConnection::dtor [" << name_ << "] destroyed";
}

void
TcpConnection::connectionEstablished() {
	loop_->assertInThread();

	assert(state_ == kConnecting);
	state_ = kConnected;
	
	// start observing read event on this socket	
	channel_->enableReading();	
	connection_callback_(shared_from_this());
}

void
TcpConnection::connectionDestroyed() {
	loop_->assertInThread();
	
	// if close_callback_ has be called, just remove channel;	
	if (state_ == kConnected) {
		channel_->disableAll();
		channel_->remove();

		state_ = kDisconnected;
		connection_callback_(shared_from_this());	
	}	
	
}

void
TcpConnection::handleRead(TimeStamp receive_time) {
	loop_->assertInThread();
	int saved_errno;
	auto n = input_buffer_.readFd(socket_->fd(), saved_errno);
	
	LOG_DEBUG << "readFd return: " << n;	

	if (n > 0) {
		message_callback_(shared_from_this(), &input_buffer_, receive_time);
	} else if (n == 0) {
		// peer close the connection
		handleClose();
	} else {
		errno = saved_errno;
		LOG_SYSERROR << "read event handle error";
		handleError();
	}

}

void
TcpConnection::handleWrite() {
	loop_->assertInThread();
	assert(channel_->isWriting());	
	
	auto n = sock::write(socket_->fd(),
						 output_buffer_.peek(),
						 output_buffer_.readable_size());
	
	if (n > 0) {
		output_buffer_.advance(n);

		if (output_buffer_.readable_size() == 0) {
			channel_->disableWriting();
			if (write_complete_callback_) {
				write_complete_callback_(shared_from_this());
			}
		}
	} else {
		LOG_SYSERROR << "write event handle error";
		handleError();
	}

}

void
TcpConnection::handleError() {
	// unsafe also ok
	//loop_->assertInThread();
	int err = sock::getsocketError(socket_->fd());
	LOG_SYSERROR << "TcpConnection [" << name_
		<< "] - [errno: " << err << "; errmsg: " << strerror_tl(err) << "]";
}

void
TcpConnection::handleClose() {
	loop_->assertInThread();
	assert(state_ == kConnected || state_ == kDisconnecting);
	
	state_ = kDisconnected;
	channel_->disableAll();
	channel_->remove();
	
	// prevent connection to be removed from TcpServer immediately	
	auto guard = shared_from_this();	
	connection_callback_(guard);
	
	// TcpServer remove connection from its connections_
	close_callback_(guard);
}
