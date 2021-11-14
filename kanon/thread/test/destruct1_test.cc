//#define NDEBUG
#include "../Thread.h"
#include "../MutexLock.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

struct A {
  void f()
  {
    zxy::MutexGuard guard(mutex_);
    printf("A::f()\n");
  }
  ~A()
  {
    zxy::MutexGuard guard(mutex_);
    printf("~A()\n");
    //sleep(1);
  }

private:
  zxy::MutexLock mutex_;
};

A* a = new A{};

using namespace kanon;

int main()
{
  Thread thr([]{
    delete a;
  });
  
  thr.start();
  sleep(1);
  // now, mutex has destroied
  // trigger assertion in lock()
  // if NDEBUG, core dump also
  a->f();
  thr.join();
}
