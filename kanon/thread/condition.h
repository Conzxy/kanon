#ifndef KANON_CONDVARIABLE_H
#define KANON_CONDVARIABLE_H

#ifndef PTHREAD_CHECK
#  define PTHREAD_CHECK
#endif

#include "kanon/util/macro.h"

#include <stdio.h>

#ifdef KANON_ON_UNIX
#  include <pthread.h>
#  include "kanon/thread/pthread_macro.h"
#else
#  include <condition_variable>
#  include <system_error>
#endif

#include "kanon/util/compiler_macro.h"
#include "kanon/util/noncopyable.h"
#include "kanon/zstl/type_traits.h"
#include "kanon/thread/mutex_lock.h"

namespace kanon {

class Condition : noncopyable {
 public:
  explicit Condition(MutexLock &mutex)
    : mutex_{mutex}
  {
#ifdef KANON_ON_UNIX
    TCHECK(pthread_cond_init(&cond_, NULL));
#endif
  }

  ~Condition()
  {
#ifdef KANON_ON_UNIX
    TCHECK(pthread_cond_destroy(&cond_));
#endif
  }

  void Wait()
  {
#ifdef KANON_ON_UNIX
    MutexLock::UnassignHolder holder(mutex_);
    TCHECK(pthread_cond_wait(&cond_, &mutex_.GetMutex()));
#else
    // Don't lock the associated mutex,
    // I prefer lock outside
    std::unique_lock<std::mutex> lock(mutex_.GetMutex(), std::defer_lock_t{});
    // No need to call wait(lock, predicate);
    cond_.wait(lock);
#endif
  }

  KANON_CORE_API bool WaitForSeconds(double seconds);

  template <typename T,
            typename = zstl::enable_if_t<std::is_integral<T>::value>>
  KANON_INLINE bool WaitForSeconds(T seconds)
  {
    return WaitForSeconds((double)seconds);
  }

  void Notify()
  {
#ifdef KANON_ON_UNIX
    TCHECK(pthread_cond_signal(&cond_));
#else
    cond_.notify_one();
#endif
  }

  void NotifyAll()
  {
#ifdef KANON_ON_UNIX
    TCHECK(pthread_cond_broadcast(&cond_));
#else
    cond_.notify_all();
#endif
  }

 private:
  MutexLock &mutex_;
#ifdef KANON_ON_UNIX
  pthread_cond_t cond_;
#else
  std::condition_variable cond_;
#endif
};

} // namespace kanon

#endif // KANON_CONDITION_H
