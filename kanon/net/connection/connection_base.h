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
#include "kanon/net/buffer.h"
#include "kanon/net/chunk_list.h"
#include "kanon/net/event.h"

#ifdef KANON_ON_WIN
#  include <winsock2.h>
#  include <windows.h>
#endif

namespace kanon {

class Socket;
class Channel;

class EventLoop;

using InputBuffer = Buffer;
using OutputBuffer = ChunkList;

template <typename D>
class ConnectionBase
  : noncopyable
  , public std::enable_shared_from_this<D> {
  // Allow TcpServer and TcpClient call the private APIs
  // that we don't exposed to user
#if 0
  friend void ChunkListOverlapSend(ChunkList &buffer, FdType fd,
                                   int &saved_errno, void *overlap);
  friend void BufferOverlapRecv(Buffer &buffer, FdType fd, int &saved_errno,
                                void *overlap);
#endif

  enum State {
    kConnecting = 0,
    kConnected,
    kDisconnecting,
    kDisconnected,
  };

  using ContextType = RawAny;
  D &GetDerived() KANON_NOEXCEPT { return static_cast<D &>(*this); }
  D const &GetDerived() const KANON_NOEXCEPT
  {
    return static_cast<D const &>(*this);
  }

  using ConnectionPtr = std::shared_ptr<D>;
  using ConnectionCallback = std::function<void(ConnectionPtr const &)>;
  using WriteCompleteCallback = std::function<bool(ConnectionPtr const &)>;
  using HighWaterMarkCallback =
      std::function<void(ConnectionPtr const &, size_t)>;
  using CloseCallback = std::function<void(ConnectionPtr const &)>;
  using MessageCallback =
      std::function<void(ConnectionPtr const &, InputBuffer &, TimeStamp)>;

 public:
  KANON_NET_NO_API ConnectionBase(EventLoop *loop, std::string const &name,
                                  int sockfd);
  KANON_NET_API ~ConnectionBase();

  //! \name write operation
  //!@{

  /**
   * \brief (Depreated)Send message in the \p buf
   *
   * \note Not thread-safe but in loop
   */
  KANON_NET_API void Send(InputBuffer &buf);

  /**
   * \brief Send message stored in the chunklist from the external
   * \note Not thread-safe but in loop
   */
  KANON_NET_API void Send(OutputBuffer &buf);

  /**
   * \brief Send message in the \p date of \p len
   *
   * \note Not thread-safe but in loop
   */
  KANON_NET_API void Send(void const *data, size_t len);

  /**
   * \brief wrapper of Send(void const*, size_t)
   */
  KANON_NET_API void Send(StringView data);

  //!@}

  //! \name close operation
  //!@{

  /**
   * \brief Half close the connection in write connection
   */
  KANON_NET_API void ShutdownWrite() KANON_NOEXCEPT;

  /**
   * \brief Close the connection regardless of peer whether send FIN or not
   */
  KANON_NET_API void ForceClose() KANON_NOEXCEPT;
  //!@}

  //! \name setter
  //!@{

  void SetMessageCallback(MessageCallback cb)
  {
    message_callback_ = std::move(cb);
  }

  void SetConnectionCallback(ConnectionCallback cb)
  {
    connection_callback_ = std::move(cb);
  }

  void SetWriteCompleteCallback(WriteCompleteCallback cb)
  {
    write_complete_callback_ = std::move(cb);
  }

  void SetHighWaterMarkCallback(HighWaterMarkCallback cb, size_t mark)
  {
    high_water_mark_ = mark;
    high_water_mark_callback_ = std::move(cb);
  }

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
  void SetContext(ContextType context) KANON_NOEXCEPT { context_ = context; }

  /**
   * \brief Disable connection continue read message from kernel space
   *
   * This is used for cooperating with HighwatermarkCallback usually
   *
   * \note
   *   If read has disabled, this is no effect
   */
  KANON_NET_API void DisbaleRead();

  /**
   * \brief Restart connection continue read message from kernel space
   *
   * This is a paired API of DisableRead()
   *
   * \note
   *   If read has enabled, this is no effect
   */
  KANON_NET_API void EnableRead();

  //!@}

  //! \name getter
  //!@{

  /**
   * \brief Get the IO loop
   */
  EventLoop *GetLoop() const KANON_NOEXCEPT { return loop_; }
  Channel *channel() KANON_NOEXCEPT { return channel_.get(); }
  /**
   * \brief Connection whether is down
   *
   * This is useful in OnConnectionCallback(),
   * we can do something when connection is established
   * or destroyed.
   */
  bool IsConnected() const KANON_NOEXCEPT { return state_ == kConnected; }
  bool IsConnecting() const KANON_NOEXCEPT { return state_ == kConnecting; }
  bool IsDisconnecting() const KANON_NOEXCEPT
  {
    return state_ == kDisconnecting;
  }
  bool IsDisconnected() const KANON_NOEXCEPT { return state_ == kDisconnected; }

  ContextType &GetContext() KANON_NOEXCEPT { return context_; }

  ContextType const &GetContext() const KANON_NOEXCEPT { return context_; }

  std::string const &GetName() const KANON_NOEXCEPT { return name_; }

  InputBuffer *GetInputBuffer() KANON_NOEXCEPT { return &input_buffer_; }

  OutputBuffer *GetOutputBuffer() KANON_NOEXCEPT { return &output_buffer_; }
  //!@}

  void SetCloseCallback(CloseCallback cb) { close_callback_ = std::move(cb); }

  // When TcpServer accept a new connection in newConnectionCallback
  KANON_NET_NO_API void ConnectionEstablished();
  // When TcpServer has removed connection from its connections_
  // ! Must not be called in event handling phase
  KANON_NET_NO_API void ConnectionDestroyed();

  KANON_NET_NO_API void SetChannel(std::unique_ptr<Channel> ch);

 protected:
#ifdef KANON_ON_WIN
  void HandleReadImmediately(size_t readn);
  void HandleWriteImmediately(size_t writen);
#endif

  void HandleRead(TimeStamp rece_time);
  void HandleLtRead(TimeStamp recv_time);
  void HandleEtRead(TimeStamp recv_time);

  void HandleWrite();
  void HandleLtWrite();
  void HandleEtWrite();

  void HandleError();
  void HandleClose();

  void CallWriteCompleteCallback();
  void SendInLoop(void const *data, size_t len);
  void SendInLoop(StringView data);
  void SendInLoopForStr(std::string &data);
  void SendInLoopForBuf(InputBuffer &buffer);
  void SendInLoopForChunkList(OutputBuffer &buffer);

  char const *State2String() const KANON_NOEXCEPT;

  // OVERLAPPED overlapped_;

  EventLoop *loop_;
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
   * Default is print the local address and peer address, connection state(TRACE
   * level) Is always override
   */
  ConnectionCallback
      connection_callback_; //!< Do something when connection is up or down

  /**
   * Called when write event is completed, a.k.a. low watermark callback \n
   * Default is empty(optional), but always specified by user.
   * \return
   *  - ture: write complete really
   *  - false: continue write in callback(e.g. pipeline)
   */
  WriteCompleteCallback
      write_complete_callback_; //!< Do something when write complete

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
   * Default is remove Connection frmo the server/client and call
   * ConnectionDestroyed() Internal callback, must not be exposed to user
   */
  CloseCallback close_callback_;

  /**
   * Context can used for binding some information
   * about a specific connnection(So, it named context)
   */
  RawAny context_;
  State state_; //!< Internal useage

#ifdef KANON_ON_WIN
 public:
  std::vector<WSABUF> wsabufs;
#endif
};

//!@}

} // namespace kanon

#endif // KANON_NET_CONNECTION_BASE_H
