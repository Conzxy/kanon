#include "kanon/log/LogFileTrigger.h"
#include "kanon/log/AsyncLogTrigger.h"
#include "kanon/thread/ThreadPool.h"

using namespace kanon;

namespace detail {

static inline void
AsyncLog_bench_impl(int num) {

  int pool_size = 10;

  ThreadPool pool{ pool_size, "AsyncLog" };
  
  pool.start(pool_size);
  
  int thread_log_num = num / pool_size;

  for (int i = 0; i != pool_size; ++i) {
    pool.run([=]() {
        for (auto i = 0; i != thread_log_num; ++i) {
          LOG_INFO << "AsyncLog_bench";
        }
    });
  }
}

}


static inline void
AsyncLog_bench(int num) {
  AsyncLogTrigger::instance("AsyncLog_bench", 200000, "/root/.log/AsyncLog_bench/");

  ::detail::AsyncLog_bench_impl(num);
}
