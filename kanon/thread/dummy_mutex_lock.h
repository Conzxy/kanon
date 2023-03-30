#ifndef KANON_DUMMY_MUTEXLOCK_H
#define KANON_DUMMY_MUTEXLOCK_H

#include "kanon/util/compiler_macro.h"
#include "kanon/util/noncopyable.h"

namespace kanon {

class DummyMutexLock : noncopyable {
 public:
  DummyMutexLock() = default;
  ~DummyMutexLock() = default;

  void Lock() {}
  void Unlock() {}
};

} // namespace kanon

#endif // KANON_DUMMY_MUTEXLOCK_H
