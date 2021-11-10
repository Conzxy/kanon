#include "kanon/log/LogFileTrigger.h"
#include "kanon/log/AsyncLogTrigger.h"
#include "kanon/thread/ThreadPool.h"
#include "kanon/time/MeasureTime.h"
#include "kanon/thread/CountDownLatch.h"

using namespace kanon;

namespace detail {

static inline void
AsyncLog_bench_impl(int num) {
  int pool_size = 10;
  CountDownLatch latch(pool_size);

  ThreadPool pool{ pool_size, "AsyncLog" };
  
  pool.start(pool_size);
  
  int thread_log_num = num / pool_size;

  for (int i = 0; i != pool_size; ++i) {
    pool.run([&latch, thread_log_num]() {
      for (auto i = 0; i != thread_log_num; ++i) {
        LOG_INFO << "AsyncLog_bench";
      }

      latch.countdown();
    });
  }

  latch.wait();
}


static inline void
LogFile_bench_impl(int num) {
  for (int i = 0; i != num; ++i) {
    LOG_INFO << "AsyncLog_bench";
  }
}

} // namespace detail

static inline void
AsyncLog_bench(int num) {
  auto& log = AsyncLogTrigger::instance("AsyncLog_bench", 200000, "/root/.log/AsyncLog_bench/").log();
  auto& latch = log.latch();
  KANON_UNUSED(latch);

  ::detail::AsyncLog_bench_impl(num);
}

static inline void
LogFile_bench(int num) {
  LogFileTrigger<> log("AsyncLog_bench", 200000, "/root/.log/LogFile_bench/");

  ::detail::LogFile_bench_impl(num);
}

#define N 10000000

int main() {
  MEASURE_TIME(AsyncLog_bench, N);
  MEASURE_TIME(LogFile_bench, N);
}