#ifndef KANON_NET_EVENTLOOP_POOL_H
#define KANON_NET_EVENTLOOP_POOL_H

#include <string>
#include <memory>
#include <vector>

#include "kanon/net/event_loop_thread.h"

namespace kanon {

class EventLoop;
class EventLoopThread;

//! \addtogroup EventLoop
//!@{

/**
 * \brief Like ThreadPool, but the elements are EventLoopThread
 * \see
 *   EventLoopThread
 */
class KANON_NET_NO_API EventLoopPool : noncopyable {
  using LoopThreadVector = std::vector<std::unique_ptr<EventLoopThread>>;
  using ThreadInitCallback = EventLoopThread::ThreadInitCallback;

 public:
  /**
   * \brief Construct a EventLoopPool object
   * \param base_loop The loop that call this(i.e. owner of the pool)
   * \param name The base name of the thread
   */
  explicit EventLoopPool(EventLoop *base_loop, std::string const &name = {});
  ~EventLoopPool() KANON_NOEXCEPT;

  //! Set pool size
  void SetLoopNum(int loop_num) KANON_NOEXCEPT { loop_num_ = loop_num; }
  int GetLoopNum() const KANON_NOEXCEPT { return loop_num_; }

  //! Ask whether pool has started
  bool IsStarted() const KANON_NOEXCEPT { return started_; }

  /**
   * \brief Start run
   * \warning
   *   Should be called once
   */
  void StartRun(ThreadInitCallback const &cb);
  void StartRun() { StartRun({}); }

  /**
   * \brief Get the next event loop in the pool based on RR(round-robin)
   * scheduling algorithm
   */
  EventLoop *GetNextLoop();

 private:
  EventLoop *base_loop_; //!< caller thread
  bool started_;         //!< used for check of invariants
  int loop_num_;         //!< The size of pool
  int next_;             //!< Index of next IO thread(used for RR)

  std::string name_;              //!< base name of event loop thread
  LoopThreadVector loop_threads_; //!< IO loopthreads
};

//!@}

} // namespace kanon

#endif // KANON_NET_EVENTLOOP_POOL_H
