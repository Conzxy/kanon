#ifndef KANON_DUMMY_MUTEXLOCK_H
#define KANON_DUMMY_MUTEXLOCK_H

#include "kanon/util/noncopyable.h"

namespace kanon {

class DummyMutexLock : noncopyable
{  
public:
  DummyMutexLock() = default;
  ~DummyMutexLock() = default;

  void lock() {}
  void unlock() {}
};

} // namespace kanon

#endif // KANON_DUMMY_MUTEXLOCK_H
