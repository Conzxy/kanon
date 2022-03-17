#include "kanon/thread/thread.h"

#include <iostream>

using namespace kanon;

int main(int argc, char* argv[])
{
  int x;
  Thread thr([&x](){
    printf("ThreadName: %s\n", kanon::CurrentThread::t_name);            
    x = 2;  
    });

  thr.StartRun();
  
  std::cout << x << '\n';
  thr.Join();
  std::cout << x << std::endl;

}
