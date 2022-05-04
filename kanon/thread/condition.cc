#include "kanon/thread/condition.h"

#include <pthread.h>
#include <time.h>
#include <errno.h>

namespace kanon{

static const int64_t Nanoseconds = 1000000000;

bool Condition::WaitForSeconds(double seconds){
  struct timespec spec;
  ::memset(&spec, 0, sizeof spec);

  ::clock_gettime(CLOCK_REALTIME, &spec);

  /*
  const int64_t Now = (spec.tv_sec+seconds)*Nanoseconds+spec.tv_nsec;

  spec.tv_sec  = static_cast<time_t>(Now/Nanoseconds);
  spec.tv_nsec = static_cast<long>(Now%Nanoseconds);
  */
  const int64_t Nanoseconds2 = seconds * Nanoseconds;
  spec.tv_sec += static_cast<time_t>((spec.tv_nsec + Nanoseconds2) / Nanoseconds);
  spec.tv_nsec = static_cast<long>((spec.tv_nsec + Nanoseconds2) % Nanoseconds);

  MutexLock::UnassignHolder holder(mutex_);
  return ETIMEDOUT == pthread_cond_timedwait(&cond_, &mutex_.GetMutex(), &spec);  
}

}//namespace kanon
