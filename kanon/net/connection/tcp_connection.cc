#include "tcp_connection.h"

#include "kanon/net/socket.h"

using namespace std;
using namespace kanon;


// TcpConnection::TcpConnection(EventLoop*  loop,
//                std::string const& name,
//                int sockfd,
//                InetAddr const& local_addr,
//                InetAddr const& peer_addr)
//   : loop_{ loop }
//   , name_{ name }
//   , socket_{ kanon::make_unique<Socket>(sockfd) }
//   , channel_{ kanon::make_unique<Channel>(loop, sockfd) }
//   , local_addr_{ local_addr }
//   , peer_addr_{ peer_addr }
//   , high_water_mark_{ kDefaultHighWatermark }
//   , state_{ kConnecting }
// {
//   // Pass raw pointer is safe here since 
//   // will disable all events when connection
//   // become disconnectioned(later, it will
//   // be destroyed)
//   channel_->SetReadCallback(std::bind(
//     &TcpConnection::HandleRead, this, _1));

//   channel_->SetWriteCallback(std::bind(
//     &TcpConnection::HandleWrite, this));

//   channel_->SetErrorCallback(std::bind(
//     &TcpConnection::HandleError, this));
  
//   channel_->SetCloseCallback(std::bind(
//     &TcpConnection::HandleClose, this));

//   LOG_TRACE_KANON << "TcpConnection::ctor [" << name_ << "] created";
  
// }

// TcpConnection::~TcpConnection() KANON_NOEXCEPT {
//   assert(state_ == kDisconnected);
//   LOG_TRACE_KANON << "TcpConnection::dtor [" << name_ << "] destroyed";
// }


TcpConnection::TcpConnection(EventLoop* loop, 
                             string const& name,
                             int sockfd,
                             InetAddr const& local_addr,
                             InetAddr const& peer_addr)
  : Base(loop, name, sockfd)
  , local_addr_(local_addr)
  , peer_addr_(peer_addr)
{
  LOG_TRACE_KANON << "TcpConnection::ctor [" << name_ << "] created";
}

TcpConnection::~TcpConnection() KANON_NOEXCEPT {
  LOG_TRACE_KANON << "TcpConnection::dtor [" << name_ << "] destroyed";
}

void TcpConnection::SetNoDelay(bool flag) KANON_NOEXCEPT
{ socket_->SetNoDelay(flag); }

void TcpConnection::SetKeepAlive(bool flag) KANON_NOEXCEPT
{ socket_->SetKeepAlive(flag); }

