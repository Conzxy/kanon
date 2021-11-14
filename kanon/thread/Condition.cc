/*
 * @version: 0.1 2021-5-24
 * @author: Conzxy
 * implement of conditial.h 
 */

#include "kanon/thread/Condition.h"
#include <pthread.h>
#include <time.h>
#include <errno.h>

namespace kanon{

bool Condition::waitForSeconds(double seconds){
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  
  const int32_t Nanoseconds = 1000000000;
  /*
  const int64_t Now = (spec.tv_sec+seconds)*Nanoseconds+spec.tv_nsec;

  spec.tv_sec  = static_cast<time_t>(Now/Nanoseconds);
  spec.tv_nsec = static_cast<long>(Now%Nanoseconds);
  */
  const int64_t Nanoseconds2 = seconds*Nanoseconds;
  spec.tv_sec += static_cast<time_t>((spec.tv_nsec+Nanoseconds2)/Nanoseconds);
  spec.tv_nsec = static_cast<long>((spec.tv_nsec+Nanoseconds2)%Nanoseconds);

  MutexLock::UnassignHolder holder(mutex_);
  return ETIMEDOUT == pthread_cond_timedwait(&cond_, &mutex_.mutex(), &spec);  
}

}//namespace kanon
