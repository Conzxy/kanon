#define KANON_TEST_THREAD

#include "kanon/thread/thread.h"
#include "kanon/thread/condition.h"

using namespace kanon;

MutexLock mu;
Condition cond(mu);
bool ready = false;

int main() {
  Thread thr([]() {
    MutexGuard g(mu);
    
    printf("Wait\n");
    while (!ready) {
      cond.Wait();
    }
  });
  
  thr.StartRun();
  
  {
    MutexGuard g(mu);
    ready = true;
    printf("Notify\n");
    cond.Notify();
  }

  thr.Join();
}
