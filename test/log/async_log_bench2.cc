#include "kanon/log/log_file.h"
#include "kanon/log/async_log.h"

#include "kanon/thread/thread_pool.h"

#include <benchmark/benchmark.h>

using namespace benchmark;
using namespace kanon;

static inline void
LogFile_bench(State& state) {
  LogFile<> lf{ "log_file", 200000, "/root/.log/async-log-bench2/" };

  SetupLogFile(lf); 

  for (auto _ : state) {
    LOG_INFO << "AsyncLogBench2";
  }
}

static inline void
AsyncLog_bench(State& state) {
  AsyncLog al("async_log", 200000, "/root/.log/async-log-bench2/");

  SetupAsyncLog(al);
  for (auto _ : state) {
    LOG_INFO << "AsyncLogBench2";
  }
}

BENCHMARK(LogFile_bench);
BENCHMARK(AsyncLog_bench);

BENCHMARK_MAIN();
