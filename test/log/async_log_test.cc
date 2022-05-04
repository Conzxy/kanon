#include "kanon/log/async_log.h"
#include "kanon/thread/thread_pool.h"

using namespace kanon;

#define NANOSECOND_PER_SECOND 1000000000

void frontThreadFunc() {
  for (int i = 0; i != 100000; ++i)
    LOG_INFO << "Async Test";
}

int main(int , char** argv) {
  AsyncLog log(::basename(argv[0]), 20000 , "/root/.log/async_log_test");

  Logger::SetOutputCallback([&log](char const* data, size_t num) {
    log.Append(data, num);
  });

  Logger::SetFlushCallback([&log]() {
    log.Flush();
  });

  log.StartRun();
  // SetupAsyncLog al(::basename(argv[0]), 20000 , "/root/.log/async_log_test");

  for (int i = 0; i < 10; ++i) {
    LOG_INFO << "async test";
  }

  struct timespec sleepTime;
  BZERO(&sleepTime, sizeof sleepTime);
  sleepTime.tv_sec = 100000;

  ::nanosleep(&sleepTime, NULL);
}
