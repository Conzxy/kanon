#ifndef KANON_NET_TCPSERVER_H
#define KANON_NET_TCPSERVER_H

#include <unordered_map>

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/type.h"
#include "kanon/mem/fixed_chunk_memory_pool.h"
// #include "kanon/util/object_pool.h"
#include "kanon/string/string_view.h"
#include "kanon/thread/atomic.h"
#include "kanon/thread/mutex_lock.h"
#include "event_loop_thread.h"

#include "kanon/net/callback.h"

namespace kanon {

class Acceptor;
class InetAddr;
class EventLoop;
class EventLoopPool;

//! \addtogroup server
//!@{

/**
 * \brief A Tcp server instance
 * 
 * User don't care of server how to accept connection and other detail. \n
 * Just write business logic in the callback and register it. \n
 * The Server can start mutilple IO event loop(thread）to process events
 * 
 * example:
 * ```cpp
 *   EventLoop loop{};
 *   InetAddr listen_addr{ 9999 };
 *   TcpServer echo_server(&loop, listen_addr, "Echo Server");
 *   echo_server.SetMessageCallback([](TcpConnnectionPtr const& conn, Buffer& buffer, TimeStamp recv_time) {
 *      // ...
 *   });
 *
 *   echo_server.SetLoopNum(1); // start with 1 IO event loop, this is option 
 *   echo_server.StartRun();
 * ```
 * \note Public class
 */
class TcpServer : noncopyable {
  using ThreadInitCallback = EventLoopThread::ThreadInitCallback;

public:
  /**
   * \param reuseport 
   */
  TcpServer(EventLoop* loop,
            InetAddr const& listen_addr,
            StringArg name,
            bool reuseport=false);
  
  ~TcpServer() noexcept;  

  //! Set the number of IO loop
  void SetLoopNum(int num) noexcept;

#if 0
  void SetPoolSize(size_t sz)
  {
    conn_pool_.SetSize(sz);
  }

  void SetPoolLimit(size_t limit)
  {
    conn_pool_.SetLimit(limit);
  }
#endif

  void SetThreadInitCallback(ThreadInitCallback cb)
  {
    init_cb_ = std::move(cb);
  } 
  /**
   * Start all IO thread to loop
   * then listen and accept connection to IO loop
   * 
   * It is harmless although this is called many times.
   * thread-safe
   */
  void StartRun() noexcept;

  //! \name getter
  //!@{

  //! Whether the server is running 
  bool IsRunning() noexcept;

  void SetConnectionCallback(ConnectionCallback cb) noexcept
  { connection_callback_ = std::move(cb); }

  void SetMessageCallback(MessageCallback cb) noexcept
  { message_callback_ = std::move(cb); }

  void SetWriteCompleteCallback(WriteCompleteCallback cb) noexcept
  { write_complete_callback_ = std::move(cb); }
  
  EventLoop* GetLoop() noexcept
  { return loop_; }
  
  //!@}
  //
  //! \name Connection Pool
  //!@{
  void EnablePool(bool enable)
  {
    enable_pool_ = enable;
  }

  void SetChunkPerBlock(size_t n) noexcept
  {
    conn_pool_.SetChunkPerBlock(n);
  }

  //!@}

private:
  typedef std::unordered_map<std::string, kanon::TcpConnectionPtr> ConnectionMap;

  EventLoop* loop_;
  std::string const ip_port_;
  std::string const name_;
  
  std::unique_ptr<Acceptor> acceptor_;  

  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;
  
  /** Store the connections */
  ConnectionMap connections_;

  /* Multi-Reactor */

  uint32_t next_conn_id;
  std::unique_ptr<EventLoopPool> pool_;
  
  /** Ensure the StartRun() be called only once */
  std::atomic<bool> start_once_;

  /** Enable the connection pool to caching memory */
  std::atomic<bool> enable_pool_;
  
  /**
   * We don't take the ThreadInitCallback as the parameter type of 
   * StartRun() since it maybe called asynchronously.
   * We must store it first
   */
  ThreadInitCallback init_cb_;
  MutexLock lock_conn_;
  
  /*
   * conn_pool_ is not a good choice to reuse connection 
   * since the memory allocation and deallocation is managed 
   * by std::shared_ptr<>
   *
   * The better approach is use memory pool as a allocator,
   * and call std::allocate_shared(allocator, args...) to control
   * the memory management
   */
  /* ObjectPoolArray<TcpConnection* const> conn_pool_; */

  /**
   * Fixed chunk memory pool.
   * The chunk size is sizeof(TcpConnection).
   */
  FixedChunkMemoryPool conn_pool_;
}; 

//!@}

} // namespace kanon

#endif // KANON_NET_TCPSERVER_H
