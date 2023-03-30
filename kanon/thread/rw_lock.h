#ifndef KANON_THREAD_RW_LOCK_H
#define KANON_THREAD_RW_LOCK_H

#include <inttypes.h>
#include <assert.h>

#include "kanon/thread/mutex_lock.h"
#include "kanon/thread/condition.h"
#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"

namespace kanon {

class KANON_CORE_API RWLock : noncopyable {
 public:
  RWLock();
  ~RWLock() KANON_NOEXCEPT;

  void RLock() KANON_NOEXCEPT;
  void RUnlock() KANON_NOEXCEPT;

  void WLock() KANON_NOEXCEPT;
  void WUnlock() KANON_NOEXCEPT;

 private:
  DISABLE_EVIL_COPYABLE(RWLock)

  MutexLock mutex_;

  /**
   * Call NotifyAll() is ok. \n
   * Allow mutilple reader awake
   */
  Condition cond_w_; //!< Wait when has_writer_ == true

  /**
   * To avoid thundering herd all writer,
   * just notify one writer
   */
  Condition cond_r_; //!< Wait when read_num_ > 0

  bool has_writer_;
  uint32_t reader_num_;
};

class RLockGuard {
 public:
  RLockGuard(RWLock &lock)
    : lock_(lock)
  {
    lock_.RLock();
  }

  ~RLockGuard() KANON_NOEXCEPT { lock_.RUnlock(); }

 private:
  DISABLE_EVIL_COPYABLE(RLockGuard)
  RWLock &lock_;
};

class WLockGuard {
 public:
  WLockGuard(RWLock &lock)
    : lock_(lock)
  {
    lock_.WLock();
  }

  ~WLockGuard() KANON_NOEXCEPT { lock_.WUnlock(); }

 private:
  DISABLE_EVIL_COPYABLE(WLockGuard)
  RWLock &lock_;
};

#define RLockGuard(x) assert(false, "This is temporary object")

#define WLockGuard(x) assert(false, "This is temporary object")

} // namespace kanon

#endif // KANON_THREAD_RW_LOCK_H
