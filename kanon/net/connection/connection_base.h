#ifndef KANON_NET_CONNECTION_BASE_H
#define KANON_NET_CONNECTION_BASE_H

#include <memory>

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/ptr.h"
// #include "kanon/util/any.h"
// #include "kanon/util/unique_any.h"
#include "kanon/util/raw_any.h"
#include "kanon/log/logger.h"

#include "kanon/net/callback.h"
#include "kanon/net/inet_addr.h"
#include "kanon/buffer/buffer.h"
#include "kanon/buffer/chunk_list.h"

namespace kanon {

class Socket;
class Channel;

class EventLoop;

using InputBuffer = Buffer;
using OutputBuffer = ChunkList;

template<typename D>
class ConnectionBase
  : noncopyable
  , public std::enable_shared_from_this<D> {
  // Allow TcpServer and TcpClient call the private APIs
  // that we don't exposed to user
  enum State {
    kConnecting = 0,
    kConnected,
    kDisconnecting,
    kDisconnected,
  };
  
  using ContextType = RawAny;
  D& GetDerived() noexcept { return static_cast<D&>(*this); }
  D const& GetDerived() const noexcept { return static_cast<D const&>(*this); }

  using ConnectionPtr = std::shared_ptr<D>;
  using ConnectionCallback    = std::function<void(ConnectionPtr const&)>;
  using WriteCompleteCallback = std::function<bool(ConnectionPtr const&)>;
  using HighWaterMarkCallback = std::function<void(ConnectionPtr const&, size_t)>;
  using CloseCallback         = std::function<void(ConnectionPtr const&)>;
  using MessageCallback       = std::function<void(ConnectionPtr const&, InputBuffer&, TimeStamp)>;

public:
  ConnectionBase(EventLoop* loop, std::string const& name, int sockfd);
  ~ConnectionBase();

  //! \name write operation
  //!@{

  /**
   * \brief (Depreated)Send message in the \p buf
   *
   * \note Not thread-safe but in loop
   */
  void Send(InputBuffer& buf);    

  /**
   * \brief Send message stored in the chunklist from the external
   * \note Not thread-safe but in loop
   */
  void Send(OutputBuffer& buf);

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

  void SetConnectionCallback(ConnectionCallback cb)
  { connection_callback_ = std::move(cb); }

  void SetWriteCompleteCallback(WriteCompleteCallback cb)
  { write_complete_callback_ = std::move(cb); }
  
  void SetHighWaterMarkCallback(HighWaterMarkCallback cb, size_t mark)
  { high_water_mark_ = mark; high_water_mark_callback_ = std::move(cb); }

  /**
   * Context can used for binding some information 
   * about a specific connnection(So, it named context)
   *
   * \warning
   *  The context don't manage the resource or lifetime of 
   *  stored object.
   *
   *  YOU must free it manaually(e.g. through GetContext())
   */
  void SetContext(ContextType context) noexcept
  {
    context_ = context;
  }

  /**
   * \brief Disable connection continue read message from kernel space
   *
   * This is used for cooperating with HighwatermarkCallback usually
   *
   * \note
   *   If read has disabled, this is no effect
   */ 
  void DisbaleRead();

  /**
   * \brief Restart connection continue read message from kernel space
   *
   * This is a paired API of DisableRead()
   *
   * \note
   *   If read has enabled, this is no effect
   */
  void EnableRead();

  //!@}

  //! \name getter
  //!@{

  /**
   * \brief Get the IO loop
   */
  EventLoop* GetLoop() const noexcept
  { return loop_; }  

  /**
   * \brief Connection whether is down
   *
   * This is useful in OnConnectionCallback(),
   * we can do something when connection is established
   * or destroyed.
   */
  bool IsConnected() const noexcept
  { return state_ == kConnected; }

  ContextType& GetContext() noexcept
  { return context_; }
  
  ContextType const& GetContext() const noexcept
  { return context_; }

  std::string const& GetName() const noexcept
  { return name_; }

  InputBuffer* GetInputBuffer() noexcept
  { return &input_buffer_; }

  OutputBuffer* GetOutputBuffer() noexcept
  { return &output_buffer_; }
  //!@}  

  void SetCloseCallback(CloseCallback cb)
  { close_callback_ = std::move(cb); }

  // When TcpServer accept a new connection in newConnectionCallback
  void ConnectionEstablished();
  // When TcpServer has removed connection from its connections_
  // ! Must not be called in event handling phase
  void ConnectionDestroyed();

protected:
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
  void SendInLoopForBuf(InputBuffer& buffer);
  void SendInLoopForChunkList(OutputBuffer& buffer);

  char const* State2String() const noexcept;

  EventLoop* loop_;
  std::string const name_;

  // Use pointer just don't want to expose them to user
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;  

  InputBuffer input_buffer_; //!< Store the message peer sent

  OutputBuffer output_buffer_; //!< Store the message local sent
  
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
  RawAny context_; 
  State state_; //!< Internal useage
};


//!@}

} // namespace kanon

#endif // KANON_NET_CONNECTION_BASE_H
