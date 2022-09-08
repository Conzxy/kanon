#include "kanon/thread/thread_local.h"
#include "kanon/thread/thread_pool.h"

#include "kanon/log/logger.h"
#include <stdio.h>

using namespace kanon;

struct Obj {
  explicit Obj(int x)
  {
    FMT_LOG_INFO("Obj(%)", x);
  }

  ~Obj()
  {
    LOG_INFO << "~Obj()";
  }
};

int main()
{
  ThreadLocal<Obj> obj;

  ThreadPool pool(10);
  for (int i = 0; i < 10; ++i) {
    pool.Push([&obj, i]() { obj.value(i); });
  }

  pool.StartRun(10);
}
