#include "mutex_lock.h"
#include "dummy_mutex_lock.h"

using namespace kanon;

int main() {
  {
  DummyMutexLock lock{};
  MutexGuardT<DummyMutexLock> guard(lock);
  }

  {
  MutexLock lock{};
  MutexGuardT<MutexLock> gurad(lock);
  }

}
