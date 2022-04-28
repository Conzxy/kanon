#include "kanon/log/async_log.h"
#include "kanon/thread/thread_pool.h"

using namespace kanon;

#define NANOSECOND_PER_SECOND 1000000000

void frontThreadFunc() {
  for (int i = 0; i != 100000; ++i)
    LOG_INFO << "Async Test";
}

int main(int , char** argv) {
  AsyncLog asyncLog{ ::basename(argv[0]), 20000 , "/root/.log/async_log_test" };
  
  SetupAsyncLog(asyncLog);

  ThreadPool pool{};
  pool.SetMaxQueueSize(10);
  pool.StartRun(10);
   
  for (int i = 0; i < 20; ++i)
    pool.Push(&frontThreadFunc);

  struct timespec sleepTime;
  BZERO(&sleepTime, sizeof sleepTime);
  sleepTime.tv_sec = 100000;

  ::nanosleep(&sleepTime, NULL);
}
