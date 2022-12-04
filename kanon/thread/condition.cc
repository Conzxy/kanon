#include "kanon/thread/condition.h"

#include <time.h>
#include <errno.h>

#if !defined(KANON_ON_UNIX)
#include <chrono>
#endif

static constexpr int64_t NANOSECONDS = 1000000000;

namespace kanon{

bool Condition::WaitForSeconds(double seconds){
#ifdef KANON_ON_UNIX
  struct timespec spec;
  ::memset(&spec, 0, sizeof spec);

  ::clock_gettime(CLOCK_REALTIME, &spec);

  /*
  const int64_t Now = (spec.tv_sec+seconds)*NANOSECONDS+spec.tv_nsec;

  spec.tv_sec  = static_cast<time_t>(Now/NANOSECONDS);
  spec.tv_nsec = static_cast<long>(Now%NANOSECONDS);
  */
  const int64_t NANOSECONDS2 = seconds * NANOSECONDS;
  spec.tv_sec += static_cast<time_t>((spec.tv_nsec + NANOSECONDS2) / NANOSECONDS);
  spec.tv_nsec = static_cast<long>((spec.tv_nsec + NANOSECONDS2) % NANOSECONDS);

  MutexLock::UnassignHolder holder(mutex_);
  return ETIMEDOUT == pthread_cond_timedwait(&cond_, &mutex_.GetMutex(), &spec);  
#else
  std::unique_lock<std::mutex> lock(mutex_.GetMutex());
  return std::cv_status::timeout == cond_.wait_for(lock, std::chrono:nanoseconds * (seconds * NANOSECONDS));
#endif
}

}//namespace kanon
