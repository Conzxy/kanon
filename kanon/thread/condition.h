#ifndef KANON_CONDVARIABLE_H
#define KANON_CONDVARIABLE_H

#ifndef PTHREAD_CHECK
#define PTHREAD_CHECK
#endif

#include <pthread.h>

#include "kanon/util/noncopyable.h"

#include "kanon/thread/pthread_macro.h"
#include "kanon/thread/mutex_lock.h"

namespace kanon{

class Condition : noncopyable {
public:
  explicit Condition(MutexLock& mutex)
    : mutex_{mutex}
  {
    TCHECK(pthread_cond_init(&cond_, NULL));
  }

  ~Condition(){
    TCHECK(pthread_cond_destroy(&cond_));
  }

  void Wait(){
    MutexLock::UnassignHolder holder(mutex_);
    TCHECK(pthread_cond_wait(&cond_, &mutex_.GetMutex()));
  }

  bool WaitForSeconds(double seconds);

  void Notify(){
    TCHECK(pthread_cond_signal(&cond_));
  }

  void NotifyAll(){
    TCHECK(pthread_cond_broadcast(&cond_));
  }

private:
  MutexLock& mutex_;  
  pthread_cond_t cond_;
};

}//namespace kanon

#endif // KANON_CONDITION_H
