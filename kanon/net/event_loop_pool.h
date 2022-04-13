#ifndef KANON_NET_EVENTLOOP_POOL_H
#define KANON_NET_EVENTLOOP_POOL_H

#include <string>

#include "kanon/net/event_loop.h"
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
class EventLoopPool : noncopyable {
  typedef std::vector<std::unique_ptr<EventLoopThread>> LoopThreadVector;
  typedef std::vector<EventLoop*> LoopVector;
public:
  /**
   * \brief Construct a EventLoopPool object
   * \param base_loop The loop that call this(i.e. owner of the pool)
   * \param name The base name of the thread
   */
  explicit EventLoopPool(EventLoop* base_loop, std::string const& name = {});
  ~EventLoopPool() noexcept;
  
  //! Set pool size 
  void SetLoopNum(int loop_num) noexcept { loop_num_ = loop_num; }

  //! Ask whether pool has started
  bool started() const noexcept { return started_; }
  
  /**
   * \brief Start run
   * \warning 
   *   Should be called once
   */
  void StartRun(); 
  
  /**
   * \brief Get the next event loop in the pool based on RR(round-robin) scheduling algorithm
   */
  EventLoop* GetNextLoop();

  /**
   * \brief Get all loops
   */
  LoopVector* GetLoops();
private: 
  
  EventLoop* base_loop_; //!< caller thread
  bool started_; //!< used for check of invariants
  int loop_num_; //!< The size of pool
  int next_; //!< Index of next IO thread(used for RR)
  
  std::string name_; //!< base name of event loop thread
  LoopThreadVector loop_threads_; //!< IO threads
  LoopVector loops_; //!< IO loops
};

//!@}

} // namespace kanon

#endif // KANON_NET_EVENTLOOP_POOL_H
