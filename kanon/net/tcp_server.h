#ifndef KANON_NET_TCPSERVER_H
#define KANON_NET_TCPSERVER_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/type.h"

#include "kanon/string/string_view.h"

#include "kanon/net/callback.h"

#include "kanon/thread/atomic.h"

namespace kanon {

class Acceptor;
class InetAddr;
class EventLoop;
class EventLoopPool;

class TcpServer : noncopyable {
public:
  TcpServer(EventLoop* loop,
        InetAddr const& listen_addr,
        StringArg name,
        bool reuseport=false);
  
  ~TcpServer() noexcept;  

  // Set the number of IO loop
  void SetLoopNum(int num) noexcept;

  // Start all IO thread to loop
  // then listen and accept connection to IO loop
  // 
  // It is harmless although this is called many times.
  // thread-safe
  void StartRun() noexcept;

  // set callback
  void SetConnectionCallback(ConnectionCallback cb) noexcept
  { connection_callback_ = std::move(cb); }

  void SetMessageCallback(MessageCallback cb) noexcept
  { message_callback_ = std::move(cb); }

  void SetWriteCompleteCallback(WriteCompleteCallback cb) noexcept
  { write_complete_callback_ = std::move(cb); }
private:
  typedef kanon::map<std::string, kanon::TcpConnectionPtr> ConnectionMap;

  // ! Must be thread-safe
  void RemoveConnection(TcpConnectionPtr const& conn);  

  EventLoop* loop_;
  std::string const ip_port_;
  std::string const name_;
  
  std::unique_ptr<Acceptor> acceptor_;  

  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;
  
  /** Store the connections */
  ConnectionMap connections_;

  /** Multi Reactor */
  uint32_t next_conn_id;
  std::unique_ptr<EventLoopPool> pool_;

  std::atomic_flag start_once_;
}; 

} // namespace kanon

#endif // KANON_NET_TCPSERVER_H
