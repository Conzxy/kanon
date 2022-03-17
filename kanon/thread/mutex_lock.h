#ifndef KANON_MUTEXLOCK_H
#define KANON_MUTEXLOCK_H

#include <pthread.h>

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"

#include "kanon/thread/pthread_macro.h"
#include "kanon/thread/current_thread.h" 


// Enable thread safety attributes only with clang.
// The attributes can be safely erased when compiling with other compilers.
// @see https://clang.llvm.org/docs/ThreadSafetyAnalysis.html
#if defined(__clang__) && (!defined(SWIG))
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   // no-op
#endif

#define CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(capability(x))

#define SCOPED_CAPABILITY \
  THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

#define GUARDED_BY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define PT_GUARDED_BY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))

#define ACQUIRED_BEFORE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquired_before(__VA_ARGS__))

#define ACQUIRED_AFTER(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquired_after(__VA_ARGS__))

#define REQUIRES(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(__VA_ARGS__))

#define REQUIRES_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(requires_shared_capability(__VA_ARGS__))

#define ACQUIRE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(__VA_ARGS__))

#define ACQUIRE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquire_shared_capability(__VA_ARGS__))

#define RELEASE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_capability(__VA_ARGS__))

#define RELEASE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_shared_capability(__VA_ARGS__))

#define RELEASE_GENERIC(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_generic_capability(__VA_ARGS__))

#define TRY_ACQUIRE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_capability(__VA_ARGS__))

#define TRY_ACQUIRE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_shared_capability(__VA_ARGS__))

#define EXCLUDES(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(__VA_ARGS__))

#define ASSERT_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(assert_capability(x))

#define ASSERT_SHARED_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_capability(x))

#define RETURN_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))

#define NO_THREAD_SAFETY_ANALYSIS \
  THREAD_ANNOTATION_ATTRIBUTE__(no_thread_safety_analysis)

namespace kanon{

class CAPABILITY("mutexlock") MutexLock : noncopyable
{
public:
  MutexLock(){
    TCHECK(pthread_mutex_init(&mutex_, NULL));
  }
  
  ~MutexLock(){
    TCHECK(pthread_mutex_destroy(&mutex_));
  }
  
  bool IsLockedInThisThread() {
    return holder_ == CurrentThread::tid();
  }
  
  void AssertLocked() {
    ASSERT(IsLockedInThisThread());
  }

  void Lock() ACQUIRE() {
    assignHolder();
    TCHECK(pthread_mutex_lock(&mutex_));
  }

  void TryLock() {
    TCHECK(pthread_mutex_trylock(&mutex_));
    unassignHolder();
  }

  void Unlock() RELEASE() {
    TCHECK(pthread_mutex_unlock(&mutex_));
    unassignHolder();
  }

  void TimedLock(timespec const* tout){
    TCHECK(pthread_mutex_timedlock(&mutex_, tout));
  }

  pthread_mutex_t& GetMutex(){
    return mutex_;
  }

private:
  friend class Condition;

  class UnassignHolder : noncopyable
  {
  public:
    explicit UnassignHolder(MutexLock& mutex)
      : mutex_(mutex)
    { mutex_.unassignHolder(); }

    ~UnassignHolder()
    { mutex_.assignHolder(); }

  private:
    MutexLock& mutex_;
  };

  void assignHolder() noexcept
  { holder_ = CurrentThread::tid(); }

  void unassignHolder() noexcept
  { holder_ = 0; }
private:
  pthread_mutex_t mutex_;
  pid_t holder_;
};

//pthread_mutex_t simple RAII wrapper
class SCOPED_CAPABILITY MutexGuard : noncopyable
{
public:
  explicit MutexGuard(MutexLock& mutex) ACQUIRE(mutex)
    : mutex_{mutex}
  { mutex_.Lock(); }

  ~MutexGuard() RELEASE()
  { mutex_.Unlock(); }
private:
  MutexLock& mutex_;
};

template<typename T>
class SCOPED_CAPABILITY MutexGuardT : noncopyable
{
public:
  explicit MutexGuardT(T& mutex) ACQUIRE(mutex)
    : mutex_(mutex)
  { mutex_.Lock(); }

  ~MutexGuardT() RELEASE()
  { mutex_.Unlock(); }
private:
  T& mutex_;
};

#define MutexGuard(x) \
  static_assert(sizeof(x) < 0, \
      "error useage of MutexGuard" \
      "(i.e. tempory object)");

#define MutexGuardT(x) \
  static_assert(sizeof(x) < 0, \
      "error useage of MutexGuard" \
      "(i.e. tempory object)");

} // namespace kanon

#endif // KANON_MUTEXLOCK_H
