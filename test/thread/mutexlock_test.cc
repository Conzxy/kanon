// #define KANON_TEST_THREAD

#include <cassert>

#include "kanon/thread/thread.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/thread/dummy_mutex_lock.h"

using namespace kanon;

MutexLock m;

#define N 10000

int main() {
#ifdef KANON_ON_UNIX
  printf("KANON_ON_UNIX\n");
#endif

  int x = 0;
  Thread thr1([&x]() {
    for (int i = 0; i < N; ++i) {
      MutexGuard g(m);
      x++;
    }
  });

  Thread thr2([&x]() {
    {
      for (int i = 0; i < N; ++i) {
        MutexGuard g(m);
        x++;
      }
    }
  });

  thr1.StartRun();
  thr2.StartRun();
  
  thr1.Join();
  thr2.Join();

  assert(x == 2 * N);
}
