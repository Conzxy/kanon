#include "mutexlock.h"
#include "DummyMutexLock.h"

using namespace zxy;

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
