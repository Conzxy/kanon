#ifndef KANON_NET_EVENTLOOP_POOL_H
#define KANON_NET_EVENTLOOP_POOL_H

#include <string>

#include "kanon/net/event_loop.h"
#include "kanon/net/event_loop_thread.h"

namespace kanon {

class EventLoop;
class EventLoopThread;

/**
 * @class EventLoopPool
 * @brief 
 * Collects IO thread loop in pool,
 * to satisfy the policy of "one loop per thread"
 */
class EventLoopPool : noncopyable {
  typedef std::vector<std::unique_ptr<EventLoopThread>> ThreadVector;
  typedef std::vector<EventLoop*> LoopVector;
public:
  explicit EventLoopPool(EventLoop* baseLoop, std::string const& name = {});
  ~EventLoopPool() noexcept;
  
  void SetLoopNum(int loopNum) noexcept { loopNum_ = loopNum; }
  bool started() const noexcept { return started_; }
  
  // Should be called once
  void StartRun(); 
  
  // Switch IO loop by using RR(round-robin) algorithm
  EventLoop* GetNextLoop();
  LoopVector* GetLoops();
private: 
  
  EventLoop* baseLoop_; //< caller thread
  bool started_; //< used for check of invariants
  int loopNum_;  
  int next_; //< next IO thread(used for RR)
  
  std::string name_;
  ThreadVector threads_; //< IO threads
  LoopVector loops_; //< IO loops
};

}

#endif // KANON_NET_EVENTLOOP_POOL_H
