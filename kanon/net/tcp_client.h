#ifndef KANON_NET_TCPCLIENT_H
#define KANON_NET_TCPCLIENT_H

#include <atomic>
#include <memory>

#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"
#include "kanon/util/ptr.h"

#include "kanon/thread/mutex_lock.h"

#include "kanon/net/callback.h"

namespace kanon {

class Connector;
class EventLoop;
class InetAddr;
class Channel;

class TcpClient;
using TcpClientPtr = std::shared_ptr<TcpClient>;

//! \addtogroup client
//!@{

/**
 * \brief A Tcp client instance
 *
 * User can connect or disconnect actively, and
 * get the connection to send message.
 * \note Public class
 */
class TcpClient : noncopyable 
                , public std::enable_shared_from_this<TcpClient> {

 protected:
  /**
   * \param loop evnet loop(usually not main thread)
   * \param serv_addr Address of server that you want to connect
   * \param name Can be empty
   */
  TcpClient(EventLoop* loop, 
            InetAddr const& serv_addr,
            std::string const& name = {});

 public:
  ~TcpClient() noexcept;

  void SetConnectionCallback(ConnectionCallback cb) noexcept
  { connection_callback_ = std::move(cb); }

  void SetMessageCallback(MessageCallback cb) noexcept
  { message_callback_ = std::move(cb); }

  void SetWriteCompleteCallback(WriteCompleteCallback cb) noexcept
  { write_complete_callback_ = std::move(cb); }
  
  //! Active connect
  void Connect();
  //! Active close
  void Disconnect();

  /**
   * \brief Stop connecting to the server
   * \note Only useful when connection isn't established successfully
   */
  void Stop();

  InetAddr const& GetServerAddr() const noexcept;

  /**
   * Enable the client retry connect when client
   * is closed by peer(But if you call disconnect
   * early, this is useless)
   */ 
  void EnableRetry() noexcept
  { retry_ = true; }

  //! \name getter
  //!@{

  /**
   * \brief Get the connection
   * 
   * I don't implement this through condition variable
   * to make caller thread sleeping when connection is not 
   * established since it will block the caller thread.
   * To GUI program, this is might not good behavior.
   *
   * If you want to block caller thread until connection 
   * is established in the other thread, you can notify
   * caller thread in the ConnectionCallback
   * \return
   *   This only return nullptr or established conection.
   * \note Thread-safe
   */
  TcpConnectionPtr GetConnection() const noexcept {
    MutexGuard guard{ mutex_ };
    return conn_;
  }

  EventLoop* GetLoop() noexcept { return loop_; }

  //!@}
 private:
  friend TcpClientPtr NewTcpClient(EventLoop*, InetAddr const&, std::string const&, bool);

  /* Register the callback
   * since can't do it in the ctor */
  void Init();
  
  /* Callback of Connector::NewConnection */
  static void NewConnection(int sockfd, InetAddr const &serv_addr, std::weak_ptr<TcpClient> const &cli);

  EventLoop* loop_;
  std::shared_ptr<Connector> connector_;

  const std::string name_;

  // callback of conn_
  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;

  /*
   * For passive close connection.
   * If peer close connection, then restart connecting.
   * 
   * If connect_ is false, indicates the user 
   * close peer actively or connector is disabled.
   * In such case, retry don't work.
   */
  std::atomic<bool> retry_;

  TcpConnectionPtr conn_ GUARDED_BY(mutex_);
  mutable MutexLock mutex_;
};

/**
 * \brief Create a tcp client in correct approach
 * \param compact Call std::make_shared if true, otherwise use `new` operator
 */
TcpClientPtr NewTcpClient(EventLoop *loop,
                          InetAddr const &serv_add,
                          std::string const &name = {},
                          bool compact=true);

//!@}
} // namespace kanon

#endif // KANON_NET_TCP_TCPCLIENT_H
