#ifndef KANON_THREAD_THREAD_LOCAL__
#define KANON_THREAD_THREAD_LOCAL__

#ifdef KANON_ON_UNIX
#include "pthread_macro.h"
#include <pthread.h>
#endif

#include <utility> // forward

namespace kanon {

#ifdef KANON_ON_UNIX

template <typename T>
class ThreadLocal {
 public:
  ThreadLocal()
  {
    TCHECK(pthread_key_create(&key_, &ThreadLocal::Destructor));
  }

  ~ThreadLocal() noexcept
  {
    TCHECK(pthread_key_delete(key_));
  }
  
  template<typename ...Args>
  T const &value(Args&&... args) const noexcept
  {
    return const_cast<ThreadLocal<T> *>(this)->value(std::forward<Args>(args)...);
  }

  /** Not thread-safe */
  template <typename ...Args>
  T &value(Args&&... args) noexcept
  {
    T *val = (T *)pthread_getspecific(key_);
    if (val == NULL) {
      val = new T(std::forward<Args>(args)...);
      TCHECK(pthread_setspecific(key_, val));
    }
    return *val;
  }

 private:
  static void Destructor(void *obj)
  {
    /* Because instantiation of member function
     * of class template is implicit(lazy instantiation).
     * If ThreadLocal don't call value(),
     * the warning is not detected */
    static_assert(sizeof(T) > 0, "T must be complete type");
    delete (T *)obj;
  }

  pthread_key_t key_;
};

#endif

} // namespace kanon

#endif // KANON_THREAD_THREAD_LOCAL__
