#include "kanon/net/TcpConnection.h"
#include "kanon/net/Socket.h"
#include "kanon/net/Channel.h"
#include "kanon/net/EventLoop.h"

#include <signal.h> // SIGPIPE, signal()

using namespace kanon;

#define DEFAULT_HIGH_WATER_MARK 64 * 1024

char const* const
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
	, high_water_mark_{ DEFAULT_HIGH_WATER_MARK }
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
	// Here shouldn't use socket_->fd(),
	// because socket maybe has destroyed when peer close early
	auto n = input_buffer_.readFd(channel_->fd(), saved_errno);
	
	LOG_DEBUG << "readFd return: " << n;	

	if (n > 0) {
		if (message_callback_) {
			message_callback_(shared_from_this(), &input_buffer_, receive_time);
		} else {
			LOG_WARN << "If user want to process message from peer, should set "
				"proper message_callback_";
		}
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
	
	auto n = sock::write(channel_->fd(),
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
	int err = sock::getsocketError(channel_->fd());
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

void
TcpConnection::shutdownWrite() KANON_NOEXCEPT {
	loop_->runInLoop([=]() {
		socket_->shutdownWrite();
	});
}

void
TcpConnection::send(void const* data, size_t len) {
	send(StringView{ 
			static_cast<char const*>(data), 
			static_cast<StringView::size_type>(len) });
}

void
TcpConnection::send(StringView data) {
	if (state_ == kConnected) {
		if (loop_->isLoopInThread()) {
			sendInLoop(data);
		} else {
			loop_->queueToLoop([=]() {
				this->sendInLoop(std::string(data.data(), data.size()));
			});
		}
	}
}

void
TcpConnection::send(Buffer& buf) {
	// user provide buf
	// use swap to avoid copy
	if (state_ == kConnected) {
		if (loop_->isLoopInThread()) {
			sendInLoop(buf.peek(), buf.readable_size());
			buf.advance(buf.readable_size());
		} else {
			output_buffer_.swap(buf);
			loop_->queueToLoop([=](){
				auto readable =output_buffer_.readable_size();
				if (readable >= high_water_mark_ &&
					high_water_mark_callback_) {
					high_water_mark_callback_(shared_from_this(), readable);
				}
				
				if (!channel_->isWriting())
					channel_->enableWriting();
			});
		}
	}
}

void
TcpConnection::sendInLoop(StringView const& data) {
	sendInLoop(data.data(), data.size());
}

void
TcpConnection::sendInLoop(void const* data, size_t len) {
	// if is not writing state, indicate @var output_buffer_ maybe is empty,
	// but also @var output_buffer_ is filled by user throught @f outputBuffer().

	ssize_t n;
	size_t remaining = len;
	if (!channel_->isWriting() && output_buffer_.readable_size() == 0) {
		LOG_DEBUG << "sendInLoop called";
		// At this case, we can write directly
		n = sock::write(socket_->fd(), data, len);
		if (n >= 0) {
			if (static_cast<size_t>(n) != len) {
				remaining -= n;	
			} else {
				if (write_complete_callback_) {
					write_complete_callback_(shared_from_this());
				}
				
				return ;
			}
		} else {
			if (errno != EAGAIN) // EWOULDBLOCK
				LOG_SYSERROR << "write unexpected error occurred";
			return ;
		}
	}

	if (remaining > 0) {
		// short write happened
		auto readable_len = output_buffer_.readable_size();
		// we will handle write complete event in @f handleWrite()
		if (readable_len + remaining >= high_water_mark_ &&
			readable_len < high_water_mark_ &&
			high_water_mark_callback_) {
			high_water_mark_callback_(shared_from_this(), readable_len + remaining);
		}
		
		output_buffer_.append(static_cast<char const*>(data)+n, remaining);
		if (!channel_->isWriting()) {
			channel_->enableWriting();
		}
	}
}

void
TcpConnection::setNoDelay(bool flag) KANON_NOEXCEPT
{ socket_->setNoDelay(flag); }

void
TcpConnection::setKeepAlive(bool flag) KANON_NOEXCEPT
{ socket_->setKeepAlive(flag); }

// When peer close connection, if you continue write the closed fd which
// will receive SIGPIPE but this signal default handler is terminal the process
// so we should ignore it
//
// @see test/SIGPIPE_test
struct IgnoreSignalPipe {
	IgnoreSignalPipe() {
		::signal(SIGPIPE, SIG_IGN);
	}
};

IgnoreSignalPipe dummy{};
