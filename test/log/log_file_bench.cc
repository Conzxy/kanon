#include "kanon/log/log_file_trigger.h"
#include "kanon/log/async_log_trigger.h"
#include "kanon/thread/thread_pool.h"

#include <benchmark/benchmark.h>

using namespace benchmark;
using namespace kanon;

static inline void
LogFile_bench(State& state) {
  LogFileTrigger<> trigger{ "LogFile_bench", 200000, "/root/.log/LogFile_bench/" };
  
  for (auto _ : state) {
    LOG_INFO << "logFile_bench";
  }
}

static inline void
AsyncLog_bench(State& state) {
  AsyncLogTrigger::instance("AsyncLog_bench", 200000, "/root/.log/AsyncLog_bench/");

  for (auto _ : state) {
    LOG_INFO << "logFile_bench";
  }
}

static inline void
AsyncLog_pool_bench(State& state) {
  AsyncLogTrigger::instance("AsyncLog_bench", 200000, "/root/.log/AsyncLog_bench/");
  
  int pool_size = 10;

  ThreadPool pool{ pool_size, "AsyncLog" };
  
  pool.StartRun(pool_size);

  for (auto i = 0; i < pool_size; ++i) { 
    pool.Push([]() {
        LOG_INFO << "LogFile_bench";
    });
  }
  
   struct timespec sleepTime;
   BZERO(&sleepTime, sizeof sleepTime);
   sleepTime.tv_sec = 100000;

   ::nanosleep(&sleepTime, NULL);
}

BENCHMARK(LogFile_bench);
BENCHMARK(AsyncLog_bench);
// BENCHMARK(AsyncLog_pool_bench);

BENCHMARK_MAIN();
