#ifndef KANON_NET_TCPCONNECTION_H
#define KANON_NET_TCPCONNECTION_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/ptr.h"
#include "kanon/util/any.h"
#include "kanon/log/logger.h"

#include "kanon/net/callback.h"
#include "kanon/net/inet_addr.h"
#include "connection_base.h"

#include <memory>

namespace kanon {

//! \addtogroup net
//!@{

/**
 * \brief Represents a tcp connection.
 *
 * User don't care detail of it, the work just to register callback as
 * following:
 *  - Message callback to process message from the peer
 *  - Highwatermark callback to do something when output buffer is too full
 *  - Connection callback to do something when connection down or up
 *  - Write complete callback to do something when write complete
 *
 * Besides,
 *  - Send message to peer(std::string, kanon::StringView, char const*,
 * kanon::Buffer)
 *  - Shutdown in write direction and close
 *  - Set context that tie with this connection
 *
 * \note
 *   Public class
 */
class TcpConnection : public ConnectionBase<TcpConnection> {
  using Base = ConnectionBase<TcpConnection>;

 protected:
 public:
  TcpConnection(EventLoop *loop, std::string const &name, int sockfd,
                InetAddr const &local_addr, InetAddr const &peer_addr);
  ~TcpConnection() noexcept;

  /**
   * \brief Create a TcpConnection instance correctly
   *
   * This is a factory method
   * \param loop owner event loop
   * \param sockfd managed socket fd
   * \param local_addr local address
   * \param peer_addr peer or remote address
   */
  template <typename Alloc>
  static TcpConnectionPtr
  NewTcpConnection(EventLoop *loop, std::string const &name, int sockfd,
                   InetAddr const &local_addr, InetAddr const &peer_addr,
                   Alloc const &a)
  {
    return kanon::AllocateSharedFromProtected<TcpConnection>(
        a, loop, name, sockfd, local_addr, peer_addr);
  }

  static TcpConnectionPtr NewTcpConnection(EventLoop *loop,
                                           std::string const &name, int sockfd,
                                           InetAddr const &local_addr,
                                           InetAddr const &peer_addr)
  {
    return NewTcpConnection(loop, name, sockfd, local_addr, peer_addr,
                            std::allocator<TcpConnection>());
  }

  static TcpConnectionPtr
  NewTcpConnectionRaw(EventLoop *loop, std::string const &name, int sockfd,
                      InetAddr const &local_addr, InetAddr const &peer_addr)
  {
    return TcpConnectionPtr(
        new TcpConnection(loop, name, sockfd, local_addr, peer_addr),
        [](TcpConnection *ptr) {
          LOG_TRACE << "Don't delete the connection to reuse";
        });
  }

  static TcpConnectionPtr
  NewTcpConnectionShared(EventLoop *loop, std::string const &name, int sockfd,
                         InetAddr const &local_addr, InetAddr const &peer_addr)
  {
    return std::make_shared<TcpConnection>(loop, name, sockfd, local_addr,
                                           peer_addr);
  }

  // void InplaceConstruct(EventLoop* loop,
  //                       std::string const& name,
  //                       int sockfd,
  //                       InetAddr const& local_addr,
  //                       InetAddr const& peer_addr)
  // {
  //   new (this) TcpConnection(loop, name, sockfd, local_addr, peer_addr);
  // }

  //! Whether disable Negele algorithm
  void SetNoDelay(bool flag) noexcept;
  //! Whether disable keep-alive timer
  void SetKeepAlive(bool flag) noexcept;

  InetAddr const &GetLocalAddr() const noexcept { return local_addr_; }

  InetAddr const &GetPeerAddr() const noexcept { return peer_addr_; }

 private:
  InetAddr const local_addr_;
  InetAddr const peer_addr_;
};

// Default connection callback.
// It is called when connection established and closed.
// Log message abont peer and local simply(trace level only)
inline void DefaultConnectionCallback(TcpConnectionPtr const &conn)
{
  LOG_TRACE_KANON << conn->GetLocalAddr().ToIpPort() << "->"
                  << conn->GetPeerAddr().ToIpPort() << " "
                  << (conn->IsConnected() ? "UP" : "DOWN");
}

} // namespace kanon

#endif // KANON_NET_TCPCONNECTION_H
