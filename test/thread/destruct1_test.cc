//#define NDEBUG
#include "kanon/thread/thread.h"
#include "kanon/thread/mutex_lock.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

using namespace kanon;

struct A {
  void f()
  {
    MutexGuard guard(mutex_);
    printf("A::f()\n");
  }
  ~A()
  {
    MutexGuard guard(mutex_);
    printf("~A()\n");
    //sleep(1);
  }

private:
  MutexLock mutex_;
};

A* a = new A{};

using namespace kanon;

int main()
{
  Thread thr([]{
    delete a;
  });
  
  thr.StartRun();
  sleep(1);
  // now, mutex has destroied
  // trigger assertion in lock()
  // if NDEBUG, core dump also
  a->f();
  thr.Join();
}
