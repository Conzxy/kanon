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

/**
 * Represents Tcp connection.
 * User don't care detail of it, you can
 * * Register message callback to process message from the peer
 * * Register highwatermark callback to do something when output buffer is too full
 * * Register connection callback to do something when connection down or up
 * * Register write complete callback to do something when write complete
 * * Send message to peer(std::string, kanon::StringView, char const*, kanon::Buffer)
 * * Shut down(usually client) and close
 * * Set context that tie with this connection
 * @note Public class
 */
class TcpConnection 
  : noncopyable
  , public std::enable_shared_from_this<TcpConnection> {
  enum State {
    kConnecting = 0x1,
    kConnected = 0x2,
    kDisconnecting = 0x4,
    kDisconnected = 0x8,
    STATE_NUM = 4,
  };
  
  static char const* const state_str_[STATE_NUM];
public:
  TcpConnection(EventLoop* loop,
          std::string const& name,
          int sockfd,
          InetAddr const& local_addr,
          InetAddr const& peer_addr);
  
  ~TcpConnection() noexcept;
  
  // !Must be thread safe  
  void Send(Buffer& buf);    
  void Send(void const* data, size_t len);
  void Send(StringView data);
  
  // half-close
  void ShutdownWrite() noexcept;
  void ForceClose() noexcept;

  // Since server thread will dispatch connection to IO thread,
  // need run some function in loop to ensure thread safe  
  EventLoop* GetLoop() const noexcept
  { return loop_; }  

  // set callback
  void SetMessageCallback(MessageCallback cb)
  { message_callback_ = std::move(cb); }

  void SetConnectionCallback(ConnectionCallback cb)
  { connection_callback_ = std::move(cb); }

  void SetWriteCompleteCallback(WriteCompleteCallback cb)
  { write_complete_callback_ = std::move(cb); }
  
  // should be called by server
  void SetCloseCallback(CloseCallback cb)
  { close_callback_ = std::move(cb); }  
  
  void SetHighWaterMarkCallback(HighWaterMarkCallback cb)
  { high_water_mark_callback_ = std::move(cb); }

  // field infomation
  bool IsConnected() const noexcept
  { return state_ == kConnected; }

  char const* State2String() const noexcept
  { return state_str_[state_]; }
  
  void SetContext(Any&& context) {
    context_ = std::move(context);
  } 
  
  void SetContext(Any const& context) {
    context_ = context;
  } 

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
  
  // set option(optional)
  void SetNoDelay(bool flag) noexcept;
  void SetKeepAlive(bool flag) noexcept;
  
  // interface for user to consume(read)
  Buffer* GetInputBuffer() noexcept
  { return &input_buffer_; }

  Buffer* GetOutputBuffer() noexcept
  { return &output_buffer_; }
  
  // When TcpServer accept a new connection in newConnectionCallback
  void ConnectionEstablished();
  // When TcpServer has removed connection from its connections_
  // ! Must not be called in event handling phase
  void ConnectionDestroyed();
private:
  void HandleError();
  void HandleClose();
  
  void SendInLoop(void const* data, size_t len);
  void SendInLoop(StringView data);

  EventLoop* loop_;
  std::string const name_;

  // Use pointer just don't want to expose them to user
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;  
  InetAddr const local_addr_;
  InetAddr const peer_addr_;
  
  State state_;

  Buffer input_buffer_;
  // FIXME Use RingBuffer better?
  Buffer output_buffer_;
  
  // Process message from @var input_buffer_  
  // * Default is empty(optional also ok, just don't process message)
  MessageCallback message_callback_;
  // Dispatch connnection to acceptor or connector
  // * Default is print the local address and peer address, connection state(TRACE level)
  ConnectionCallback connection_callback_;

  // Called when write event is completed
  // a.k.a. low_water_mark_callback_
  // * Default is empty(optional)
  WriteCompleteCallback write_complete_callback_;  
  
  // Avoid so much data filling the kernel input/output buffer
  // Note: the callback only be called in rising edge
  // * Default is empty(optional)
  HighWaterMarkCallback high_water_mark_callback_;
  size_t high_water_mark_;
  
  // Context can used for binding some information 
  // about a specific connnection(So, it named context)
  Any context_;

  // Only be called by server
  // * Default is remove Connection frmo the server and call ConnectionDestroyed()
  // * Internal callback, must not be exposed to user
  CloseCallback close_callback_;
};

// Default connection callback.
// It is called when connection established and closed.
// Log message abont peer and local simply(trace level only)
inline void DefaultConnectionCallback(TcpConnectionPtr const& conn) {
  LOG_TRACE << conn->GetLocalAddr().ToIpPort() << "->"
    << conn->GetPeerAddr().ToIpPort() << " "
    << (conn->IsConnected() ? "UP" : "DOWN");
}

} // namespace kanon

#endif // KANON_NET_TCPCONNECTION_H