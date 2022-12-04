// #define KANON_TEST_THREAD
#include "kanon/thread/thread.h"

#include <iostream>

using namespace kanon;

int main(int argc, char* argv[])
{
#ifdef KANON_ON_UNIX
  printf("KANON_ON_UNIX is defined");
#endif

  int x;
  Thread thr([&x](){
    printf("ThreadName: %s\n", kanon::CurrentThread::t_name);            
    x = 2;
  });
  
  Thread thr2([&x]() {
    printf("ThreadName: %s\n", kanon::CurrentThread::t_name);
    x = 3;
  });

  thr.StartRun();
  thr2.StartRun();
  
  printf("x = %d\n", x);
  thr.Join();
  thr2.Join();
  printf("x = %d\n", x);
}
