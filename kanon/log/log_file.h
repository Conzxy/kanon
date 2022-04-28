#ifndef KANON_LOGFILE_H
#define KANON_LOGFILE_H

#include <memory>
#include <time.h>
#include <sys/time.h>
#include <string>
#include <limits.h>
#include <type_traits> // std::conditional

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/measure_time.h"
#include "kanon/string/string_view.h"

#include "kanon/thread/dummy_mutex_lock.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/thread/thread.h"
#include "kanon/thread/condition.h"

#include "kanon/process/process_info.h"

#include "kanon/log/logger.h"
#include "kanon/log/append_file.h"

namespace kanon {

/**
 * \brief Append log to file
 * 
 * The file is rolled automatically when the filesize reach threshold
 * or new day
 * \note should be used by Logger
 */
template<bool ThreadSafe = false>
class LogFile : noncopyable {
public:

  /**
   * \param basename Filename of this process
   * \param roll_size File size to roll new file
   * \param prefix Prefix of directory name that store log files(default: current directory)
   * \param log_file_num The threshold to remove old log files(default: UINT_MAX, i.e. don't remove)
   * \param roll_interval Interval to roll new file in seconds(default: 86400, i.e. 1 day)
   * \param flush_interval Interval of flushing contents in buffer to file in seconds(default: 3s)
   */
  LogFile(StringView basename, 
          size_t roll_size, 
          StringView prefix = StringView{ },
          size_t log_file_num = UINT_MAX,
          size_t roll_interval = kRollPerSeconds_,
          size_t flush_interval = 3);

  ~LogFile() noexcept;
  
  void Append(char const* data, size_t num) noexcept;
  void Flush() noexcept;
private:
  void RollFile();
  std::string GetLogFilename(time_t& now);  

  static std::string FormatTime() noexcept;
private:
  std::string basename_; //!< The basename of process
  size_t roll_size_; //!< when file size > rollSize, roll new file

  time_t start_of_period_; //!< over the start of period, roll new file
  time_t roll_interval_;
  time_t last_roll_; //!< remember last roll time to avoid roll same file at the same time

  time_t last_flush_;
  time_t flush_interval_; //!< if now - lastFlush > flushInterval, flush buffer to file
  
  // lock or dummy lock
  using MutexPolicy = typename std::conditional<ThreadSafe, MutexLock, DummyMutexLock>::type;

  MutexPolicy lock_;
  std::unique_ptr<AppendFile> file_;
  
  std::string prefix_; //!< directory that store log file

  size_t log_file_num_;
  std::vector<std::string> log_files_;
  std::vector<std::string> log_files_dup_;
  Thread remove_thr_;
  Condition remove_cond_;
  MutexLock remove_mtx_;
  bool quit_remove_thr_;

  static constexpr uint32_t kRollPerSeconds_ = 24 * 60 * 60; //or 86400
};

template<bool ThreadSafe>
void SetupLogFile(LogFile<ThreadSafe>& lf) {
  Logger::SetOutputCallback([&lf](char const* data, size_t num) {
    lf.Append(data, num);
  });

  Logger::SetFlushCallback([&lf]() {
    lf.Flush();
  });
}


} // namespace kanon

#endif // KANON_LOGFILE_H
