#ifndef KANON_NET_TCPCLIENT_H
#define KANON_NET_TCPCLIENT_H

#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"
#include "kanon/util/unique_ptr.h"
#include "kanon/net/callback.h"
#include "kanon/thread/MutexLock.h"

#include <atomic>

namespace kanon {

class Connector;
class EventLoop;
class InetAddr;
class Channel;

typedef std::unique_ptr<Connector> ConnectorPtr;

class TcpClient : noncopyable {
public:
  TcpClient(
    EventLoop* loop, 
    InetAddr const& servAddr,
    std::string const& name = {});
  ~TcpClient() KANON_NOEXCEPT;

  void setConnectionCallback(ConnectionCallback cb) KANON_NOEXCEPT
  { connection_callback_ = std::move(cb); }

  void setMessageCallback(MessageCallback cb) KANON_NOEXCEPT
  { message_callback_ = std::move(cb); }

  void setWriteCompleteCallback(WriteCompleteCallback cb) KANON_NOEXCEPT
  { write_complete_callback_ = std::move(cb); }

  // active connect
  void connect() KANON_NOEXCEPT;
  // active close
  void disconnect() KANON_NOEXCEPT;
  
  void stop() KANON_NOEXCEPT;
  
  void enableRetry() KANON_NOEXCEPT
  { retry_ = true; }

  TcpConnectionPtr connection() const KANON_NOEXCEPT {
    return conn_;
  }

private:
  EventLoop* loop_;
  ConnectorPtr connector_;

  const std::string name_;

  // set for connection
  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;

  // for active close connection
  // shutdown write and set connect_ to false
  std::atomic<bool> connect_;
  // for passive close connection
  // if peer close connection, if restart
  std::atomic<bool> retry_;

  TcpConnectionPtr conn_;
  mutable MutexLock mutex_;
};

} // namespace kanon

#endif // KANON_NET_TCP_TCPCLIENT_H
