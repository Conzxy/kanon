#ifndef KANON_NET_TCPSERVER_H
#define KANON_NET_TCPSERVER_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/type.h"
#include "kanon/string/string_view.h"
#include "kanon/net/callback.h"
#include "kanon/thread/Atomic.h"


#include <string>

namespace kanon {

// set Acceptor to std::unique_ptr to avoid expose Socket and Channel to user
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
  
  ~TcpServer() KANON_NOEXCEPT;  

  // Set the number of IO loop
  void setLoopNum(int num) KANON_NOEXCEPT;

  // Start all IO thread to loop
  // then listen and accept connection to IO loop
  // 
  // It is harmless although this is called many times.
  // thread-safe
  void start() KANON_NOEXCEPT;

  // set callback
  void setConnectionCallback(ConnectionCallback cb) KANON_NOEXCEPT
  { connection_callback_ = std::move(cb); }

  void setMessageCallback(MessageCallback cb) KANON_NOEXCEPT
  { message_callback_ = std::move(cb); }

  void setWriteCompleteCallback(WriteCompleteCallback cb) KANON_NOEXCEPT
  { write_complete_callback_ = std::move(cb); }
private:
  typedef kanon::map<std::string, kanon::TcpConnectionPtr> ConnectionMap;

  void removeConnection(TcpConnectionPtr const& conn);  

  EventLoop* loop_;
  std::string const ip_port_;
  std::string const name_;
  
  std::unique_ptr<Acceptor> acceptor_;  

  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;
  
  uint32_t next_conn_id;
  ConnectionMap connections_;

  std::unique_ptr<EventLoopPool> pool_;

  AtomicInt32 count_;
}; 

} // namespace kanon

#endif // KANON_NET_TCPSERVER_H
