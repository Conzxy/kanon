#ifndef KANON_ASYNC_LOG_H
#define KANON_ASYNC_LOG_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/unique_ptr.h"
#include "kanon/string/string_view.h"
#include "kanon/string/lexical_stream.h"
#include "kanon/thread/Thread.h"
#include "kanon/thread/Condition.h"
#include "kanon/thread/MutexLock.h"
#include "kanon/thread/CountDownLatch.h"

#include <vector>
#include <atomic>
#include <stdint.h>

namespace kanon {

/**
 * @class AsyncLog
 * @brief 
 * Log message to specified devices or files asynchronously
 * @warning
 * If main thread exits, then backthread which write message to disk will also stop.
 * Therefore, it is best that this used for such service which is long-running
 * (Fortunately, server is usually long-running).
 * @note This should used by Logger
 * @note Must be thread safe
 */
class AsyncLog : noncopyable {
public:
  AsyncLog(
      StringView basename,
      size_t rollSize,
      StringView prefix = "",
      size_t flushInterval = 3,
      size_t rollLine = 1024);
	
	~AsyncLog() KANON_NOEXCEPT;
  
  void append(char const* data, size_t num) KANON_NOEXCEPT;
  void flush() KANON_NOEXCEPT;

  CountDownLatch& latch() KANON_NOEXCEPT
  { return latch_; }

  void start();
  void stop() KANON_NOEXCEPT;

private:
  void threadFunc();

  typedef detail::FixedBuffer<LARGE_FIXEDBUFFER_SIZE> Buffer;
  typedef std::unique_ptr<Buffer> BufferUPtr;

  typedef std::vector<BufferUPtr> Buffers;

  // forward parameter to LogFile
  StringView basename_;
  size_t rollSize_;
  size_t flushInterval_;
  size_t rollLine_;

  StringView prefix_;	
  std::atomic<bool> running_;

  // use "Multiplex Buffering" technique
  // @see "Mutiple Buffering" entry of Wikipedia
  BufferUPtr currentBuffer_;
  BufferUPtr nextBuffer_;
  Buffers buffers_;

// mutexlock used to synchronize front threads(i.e. writers)
  MutexLock mutex_;

// If @var buffers_ is empty, back thread sleeping
  Condition notEmpty_;

  // backThread log buffer to disk
  Thread backThread_;

  // as barrier to avoid main thread destroy it early
  CountDownLatch latch_;
};

} // namespace kanon

#endif // KANON_ASYNC_LOG_H