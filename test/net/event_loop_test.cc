#include "kanon/net/event_loop.h"
#include "kanon/thread/thread.h"
#include "kanon/thread/current_thread.h"
#include <stdio.h>

using namespace kanon;

std::unique_ptr<EventLoop> g_loop;

void threadFunc()
{
  printf("threadFunc: %s\n", CurrentThread::t_tidString);
  g_loop->StartLoop();
}

int main()
{
  printf("main: %s\n", CurrentThread::t_tidString);

  g_loop.reset(new EventLoop{});
  Thread thr{&threadFunc};
  thr.StartRun();
  thr.Join();
}
