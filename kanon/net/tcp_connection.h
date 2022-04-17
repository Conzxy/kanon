#ifndef KANON_NET_TCPCONNECTION_H
#define KANON_NET_TCPCONNECTION_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/ptr.h"
#include "kanon/util/any.h"
#include "kanon/log/logger.h"

#include "kanon/net/callback.h"
#include "kanon/net/inet_addr.h"
#include "kanon/net/buffer.h"

namespace kanon {

// Don't expose to user
class Socket;
class Channel;

class EventLoop;
class TcpServer;
class TcpClient;

//! \addtogroup net
//!@{

/**
 * \brief Represents a tcp connection.
 *
 * User don't care detail of it, the work just to register callback as following:
 *  - Message callback to process message from the peer
 *  - Highwatermark callback to do something when output buffer is too full
 *  - Connection callback to do something when connection down or up
 *  - Write complete callback to do something when write complete
 * Besides,
 *  - Send message to peer(std::string, kanon::StringView, char const*, kanon::Buffer)
 *  - Shutdown in write direction and close
 *  - Set context that tie with this connection
 *
 * \note 
 *   Public class
 */
class TcpConnection 
  : noncopyable
  , public std::enable_shared_from_this<TcpConnection> {
  // Allow TcpServer and TcpClient call the private APIs
  // that we don't exposed to user
  friend class TcpServer;
  friend class TcpClient;

  enum State {
    kConnecting = 0,
    kConnected,
    kDisconnecting,
    kDisconnected,
  };
  
public:
  ~TcpConnection() noexcept;

  /**
   * Shouldn't call this. To call NewTcpConnection().
   * Can't set this be private member
   * since std::make_shared<> need access ctor
   */
  TcpConnection(EventLoop* loop,
                std::string const& name,
                int sockfd,
                InetAddr const& local_addr,
                InetAddr const& peer_addr);

  /**
   * \brief Create a TcpConnection instance correctly
   * 
   * This is a factory method
   * \param loop owner event loop
   * \param sockfd managed socket fd
   * \param local_addr local address
   * \param peer_addr peer or remote address
   */
  static TcpConnectionPtr NewTcpConnection(EventLoop* loop, 
                                           std::string const& name,
                                           int sockfd,
                                           InetAddr const& local_addr,
                                           InetAddr const& peer_addr)
  { return std::make_shared<TcpConnection>(loop, name, sockfd, local_addr, peer_addr); }
  

  //! \name write operation
  //!@{

  /**
   * \brief Send message in the \p buf
   *
   * \note Not thread-safe but in loop
   */
  void Send(Buffer& buf);    

  /**
   * \brief Send message in the \p date of \p len
   *
   * \note Not thread-safe but in loop
   */
  void Send(void const* data, size_t len);

  /**
   * \brief wrapper of Send(void const*, size_t)
   */
  void Send(StringView data);

  //!@}  

  //! \name close operation
  //!@{

  /**
   * \brief Half close the connection in write connection
   */
  void ShutdownWrite() noexcept;

  /**
   * \brief Close the connection regardless of peer whether send FIN or not
   */
  void ForceClose() noexcept;
  //!@}


  //! \name setter
  //!@{

  void SetMessageCallback(MessageCallback cb)
  { message_callback_ = std::move(cb); }

  void SetWriteCompleteCallback(WriteCompleteCallback cb)
  { write_complete_callback_ = std::move(cb); }
  
  void SetCloseCallback(CloseCallback cb)
  { close_callback_ = std::move(cb); }
  
  void SetHighWaterMarkCallback(HighWaterMarkCallback cb, size_t mark)
  { high_water_mark_ = mark; high_water_mark_callback_ = std::move(cb); }

  /**
   * Context can used for binding some information 
   * about a specific connnection(So, it named context)
   */
  void SetContext(Any&& context) { context_ = std::move(context); }
  
  void SetContext(Any const& context) { context_ = context; } 

  //! Whether disable Negele algorithm
  void SetNoDelay(bool flag) noexcept;
  //! Whether disable keep-alive timer
  void SetKeepAlive(bool flag) noexcept;

  //!@}

  //! \name getter
  //!@{

  /**
   * Accept thread(OR main thread) will dispatch connection to IO thread
   * to ensure one loop per thread
   * \return
   *   The event loop where acceptor in(Usually, this is main thread)
   */
  EventLoop* GetLoop() const noexcept
  { return loop_; }  

  /**
   * \brief Connection whether is down
   * This is useful in OnConnectionCallback(),
   * we can do something when connection is established
   * or destroyed.
   */
  bool IsConnected() const noexcept
  { return state_ == kConnected; }

  Any& GetContext() noexcept
  { return context_; }
  
  Any const& GetContext() const noexcept
  { return context_; }

  std::string const& GetName() const noexcept
  { return name_; }

  InetAddr const& GetLocalAddr() const noexcept
  { return local_addr_; }

  InetAddr const& GetPeerAddr() const noexcept
  { return peer_addr_; }
  
  Buffer* GetInputBuffer() noexcept
  { return &input_buffer_; }

  Buffer* GetOutputBuffer() noexcept
  { return &output_buffer_; }
  //!@}  

private:
  void HandleRead(TimeStamp rece_time);
  void HandleLtRead(TimeStamp recv_time);
  void HandleEtRead(TimeStamp recv_time);

  void HandleWrite();
  void HandleLtWrite();
  void HandleEtWrite();

  void HandleError();
  void HandleClose();

  void CallWriteCompleteCallback(); 
  void SendInLoop(void const* data, size_t len);
  void SendInLoop(StringView data);
  void SendInLoopForStr(std::string& data);
  void SendInLoopForBuf(Buffer& buffer);

  void SetConnectionCallback(ConnectionCallback cb)
  { connection_callback_ = std::move(cb); }

  // When TcpServer accept a new connection in newConnectionCallback
  void ConnectionEstablished();
  // When TcpServer has removed connection from its connections_
  // ! Must not be called in event handling phase
  void ConnectionDestroyed();

  char const* State2String() const noexcept;

  EventLoop* loop_;
  std::string const name_;

  // Use pointer just don't want to expose them to user
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;  
  InetAddr const local_addr_;
  InetAddr const peer_addr_;

  Buffer input_buffer_; //!< Store the message peer sent

  /**
   * TODO(conzxy/output_buffer) use linked list to achieve zero-copy
   */
  Buffer output_buffer_; //!< Store the message local sent
  
  /**
   * Default is empty(optional also ok, just don't process message and discard)
   * But this is always specified by user
   */ 
  MessageCallback message_callback_; //!< Process message from input_buffer_ 

  /**
   * Default is print the local address and peer address, connection state(TRACE level)
   * Is always override
   */
  ConnectionCallback connection_callback_; //!< Do something when connection is up or down

  /**
   * Called when write event is completed, a.k.a. low watermark callback \n
   * Default is empty(optional), but always specified by user.
   * \return 
   *  - ture: write complete really
   *  - false: continue write in callback(e.g. pipeline)
   */
  WriteCompleteCallback write_complete_callback_;  //!< Do something when write complete

  /**
   * Avoid so much data filling the kernel input/output buffer
   * Default is empty(optional)
   * Is always use with write_complete_callback_
   * \warning
   *   the callback only be called in rising edge
   */
  HighWaterMarkCallback high_water_mark_callback_;
  size_t high_water_mark_;
  
  /**
   * Default is remove Connection frmo the server/client and call ConnectionDestroyed()
   * Internal callback, must not be exposed to user
   */
  CloseCallback close_callback_;

  /**
   * Context can used for binding some information 
   * about a specific connnection(So, it named context)
   */
  Any context_;
  
  State state_; //!< Internal useage
};

// Default connection callback.
// It is called when connection established and closed.
// Log message abont peer and local simply(trace level only)
inline void DefaultConnectionCallback(TcpConnectionPtr const& conn) {
  LOG_TRACE << conn->GetLocalAddr().ToIpPort() << "->"
    << conn->GetPeerAddr().ToIpPort() << " "
    << (conn->IsConnected() ? "UP" : "DOWN");
}

//!@}
} // namespace kanon

#endif // KANON_NET_TCPCONNECTION_H
