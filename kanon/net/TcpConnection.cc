#include "kanon/net/TcpConnection.h"
#include "kanon/net/Socket.h"
#include "kanon/net/Channel.h"

using namespace kanon;

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
{
}

TcpConnection::~TcpConnection() KANON_NOEXCEPT = default;


