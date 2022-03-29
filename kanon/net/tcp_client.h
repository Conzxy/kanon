#ifndef KANON_NET_TCPCLIENT_H
#define KANON_NET_TCPCLIENT_H

#include <atomic>

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

class TcpClient : noncopyable {
public:
  TcpClient(
    EventLoop* loop, 
    InetAddr const& servAddr,
    std::string const& name = {});
  ~TcpClient() noexcept;

  void SetConnectionCallback(ConnectionCallback cb) noexcept
  { connection_callback_ = std::move(cb); }

  void SetMessageCallback(MessageCallback cb) noexcept
  { message_callback_ = std::move(cb); }

  void SetWriteCompleteCallback(WriteCompleteCallback cb) noexcept
  { write_complete_callback_ = std::move(cb); }

  // active connect
  void Connect() noexcept;
  // active close
  void Disconnect() noexcept;
  
  void Stop() noexcept;
  
  void EnableRetry() noexcept
  { retry_ = true; }

  TcpConnectionPtr GetConnection() const noexcept {
    MutexGuard guard{ mutex_ };
    return conn_;
  }

  EventLoop* GetLoop() noexcept { return loop_; }
private:
  EventLoop* loop_;
  std::unique_ptr<Connector> connector_;

  const std::string name_;

  // callback of conn_
  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;

  // For active close connection
  // Shutdown write and set connect_ to false
  std::atomic<bool> connect_;

  // For passive close connection
  // If peer close connection, then restart
  std::atomic<bool> retry_;

  TcpConnectionPtr conn_ GUARDED_BY(mutex_);
  mutable MutexLock mutex_;
};

} // namespace kanon

#endif // KANON_NET_TCP_TCPCLIENT_H
