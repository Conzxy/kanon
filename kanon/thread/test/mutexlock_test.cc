#include "MutexLock.h"
#include "DummyMutexLock.h"

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
