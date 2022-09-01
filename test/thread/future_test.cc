#include "kanon/thread/thread.h"
#include <future>

#include <iostream>

using namespace kanon;

int main()
{
  std::promise<int> p;
  Thread thr([&p]() {
    std::cout << "set_value 20\n";
    p.set_value(20);
    std::cout << "set value 20 finish\n";
  });
  thr.StartRun();

  std::future<int> fu = p.get_future();

  std::cout << fu.get() << std::endl;
  std::cout << "future" << std::endl;
  thr.Join();
}
