#include <assert.h>
#include <limits.h>

#include "kanon/thread/rw_lock.h"
#include "kanon/thread/mutex_lock.h"

static constexpr uint64_t MAX_READER_NUM = UINT64_MAX;

namespace kanon {

RWLock::RWLock()
  : cond_w_(mutex_)
  , cond_r_(mutex_)
  , has_writer_(false)
  , reader_num_(0)
{
}

RWLock::~RWLock() noexcept = default;

void RWLock::RLock() noexcept
{
  MutexGuard g(mutex_);
  
  while (has_writer_ || reader_num_ == MAX_READER_NUM) {
    cond_w_.Wait();
  }

  ++reader_num_;
}

void RWLock::RUnlock() noexcept
{
  MutexGuard g(mutex_);

  --reader_num_;

  // If there is a writer waiting, first notify writer
  // to prevent more read enter.
  if (has_writer_) {
    if (reader_num_ == 0) {
      cond_r_.Notify();
    }
  }
  else {
    // There is must no writer waiting!
    // To avoid awake reader up and sleep again.
    //
    // If reader_num_ has reach the maximum,
    // there are some readers waiting, notify one when 
    // no writer waiting(Not notify all to avoid reader_num_
    // reach the maximum then waiting again)
    if (reader_num_ == MAX_READER_NUM - 1) {
      cond_w_.Notify();
    }
  }
}

void RWLock::WLock() noexcept
{
  MutexGuard g(mutex_);

  // First check has_writer to prevent
  // more reader locking successfully
  while (has_writer_) {
    cond_w_.Wait();
  }

  has_writer_ = true;

  while (reader_num_ > 0) {
    cond_r_.Wait();
  }
}

void RWLock::WUnlock() noexcept
{
  MutexGuard g(mutex_);

  has_writer_ = false;

  cond_w_.NotifyAll();
}

} // namespace kanon
