#ifndef KANON_ASYNC_LOG_H
#define KANON_ASYNC_LOG_H

#include <stdint.h>
#include <vector>
#include <atomic>
#include <limits.h>

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/ptr.h"

#include "kanon/string/type.h"
#include "kanon/string/string_view.h"
#include "kanon/string/stream_common.h"

#include "kanon/thread/thread.h"
#include "kanon/thread/condition.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/thread/count_down_latch.h"

#include "kanon/log/logger.h"

namespace kanon {

/**
 * \brief Log message to specified devices or files asynchronously
 * \warning
 *   If main thread exits, then backthread which write message to disk will also stop.
 *   Therefore, it is best that this used for such service which is long-running
 *   (Fortunately, server is usually long-running).
 * \note 
 *   This should used by Logger.
 *   Must be thread safe
 */
class AsyncLog : noncopyable {
public:
  /** 
   * \see LogFile
   */
  AsyncLog(
      StringView basename,
      size_t roll_size,
      StringView prefix = "",
      size_t log_file_num = UINT_MAX,
      size_t roll_interval = 86400,
      size_t flush_interval = 3);
  
  ~AsyncLog() noexcept;
  
  void Append(char const* data, size_t num) noexcept;
  void Flush() noexcept;

  void StartRun();
  void Stop() noexcept;

private:
  typedef detail::LargeFixedBuffer Buffer;

  // Don't expose the FixedBuffer
  typedef std::unique_ptr<Buffer> BufferUPtr;
  typedef std::vector<BufferUPtr> Buffers;

  // Forward parameter to LogFile
  std::string basename_;
  size_t roll_size_;
  std::string prefix_;  
  size_t log_file_num_;
  size_t roll_interval_;
  size_t flush_interval_;

  std::atomic<bool> running_;

  /**
   * use "Multiplex Buffering" technique
   * \see "Multiplex Buffering" entry of Wikipedia
   */
  BufferUPtr current_buffer_;
  BufferUPtr next_buffer_;
  Buffers buffers_;
  Buffers buffers_dup_;

  // mutexlock used to synchronize front threads(i.e. writers)
  MutexLock mutex_;

  // If buffers_ is empty, back thread sleeping
  Condition not_empty_;

  // back thread log buffer to disk
  Thread back_thr_;

  // As a barrier to avoid main thread destroy it early
  CountDownLatch latch_;
};

/**
 * \warning 
 *  -- construct before any logic, e.g. the first statement in main()
 */
inline void SetupAsyncLog(StringView basename, 
                          size_t roll_size,
                          StringView prefix = "",
                          size_t log_file_num = UINT_MAX,
                          size_t roll_interval = 86400,
                          size_t flush_interval = 3) {
  static AsyncLog al(basename, roll_size, prefix, log_file_num, roll_interval, flush_interval);

  Logger::SetFlushCallback([](){
    al.Flush();
  });

  Logger::SetOutputCallback([](char const* data, size_t len) {
    al.Append(data, len);
  });

  al.StartRun();
}

} // namespace kanon

#endif // KANON_ASYNC_LOG_H
