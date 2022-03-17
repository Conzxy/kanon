#include "kanon/log/log_file_trigger.h"
#include "kanon/log/async_log_trigger.h"
#include "kanon/thread/thread_pool.h"
#include "kanon/util/measure_time.h"
#include "kanon/thread/count_down_latch.h"

using namespace kanon;

namespace detail {

static inline void
AsyncLog_bench_impl(int num) {
  int pool_size = 10;
  CountDownLatch latch(pool_size);

  ThreadPool pool{ pool_size, "AsyncLog" };
  
  pool.StartRun(pool_size);
  
  int thread_log_num = num / pool_size;

  for (int i = 0; i != pool_size; ++i) {
    pool.Push([&latch, thread_log_num]() {
      for (auto i = 0; i != thread_log_num; ++i) {
        LOG_INFO << "AsyncLog_bench";
      }

      latch.Countdown();
    });
  }

  latch.Wait();
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