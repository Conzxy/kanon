#ifndef KANON_LOGFILE_H
#define KANON_LOGFILE_H

#include <memory>
#include <time.h>
#include <sys/time.h>
#include <string>
#include <type_traits> // std::conditional

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/string/string_view.h"
#include "kanon/thread/dummy_mutex_lock.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/process/process_info.h"

#include "kanon/log/logger.h"
#include "kanon/log/append_file.h"

namespace kanon {

/**
 * @brief just roll file and write to file
 * @note should be used by Logger
 */
template<bool ThreadSafe = false>
class LogFile : noncopyable
{
public:
  LogFile(StringView basename, 
      size_t rollSize, 
      StringView prefix = StringView{ },
      size_t flushInterval = 3,
      size_t rollLine = 1024);

  ~LogFile() noexcept;
  
  void Append(char const* data, size_t num) noexcept;
  void flush() noexcept;
private:
  void rollFile();
  static std::string formatTime() noexcept;
  std::string getLogFileName(time_t& now);  

private:
  StringView basename_;
  size_t rollSize_; // when file size > rollSize, roll new file
  size_t rollLine_; // when file line count > rollLine, also roll new file
  size_t countLine_; // count of written line

  time_t startOfPeriod_; // over the start of period, roll new file
  time_t lastRoll_; // remember last roll time to avoid roll same file at the same time
  time_t lastFlush_;
  time_t flushInterval_; // if now - lastFlush > flushInterval, flush buffer to file
  
  // lock or dummy lock
  typedef typename std::conditional<ThreadSafe, MutexLock, DummyMutexLock>::type 
    MutexPolicy;

  MutexPolicy lock_;
  std::unique_ptr<AppendFile> file_;
  
  // FIXME std::string? 
  StringView prefix_;

  // seconds for one day
  static constexpr uint32_t kRollPerSeconds_ = 24 * 60 * 60; //or 86400
};

template<bool T>
LogFile<T>::LogFile(StringView basename,
         size_t rollSize, 
         StringView prefix,
         size_t flushInterval,
         size_t rollLine)
  : basename_(basename)
  , rollSize_(rollSize)
  , rollLine_(rollLine)
  , countLine_(0)
  , startOfPeriod_(0)
  , lastRoll_(0)
  , lastFlush_(0)
  , flushInterval_(static_cast<time_t>(flushInterval))
  , lock_()
  , prefix_(prefix)
{
  assert(basename.find('/') == StringView::npos);
  Logger::SetColor(false);
  rollFile();  
}

template<bool T>
inline LogFile<T>::~LogFile() noexcept = default;

template<bool T>
void LogFile<T>::Append(char const* data, size_t num) noexcept {
  MutexGuardT<MutexPolicy> guard(lock_);
  
  file_->Append(data, num);

  if (file_->writtenBytes() > rollSize_)
    rollFile();
  else {
    ++countLine_;
    if (countLine_ >= rollLine_) {
      countLine_ = 0;
      // second precision
      auto now = ::time(NULL);
  
      // @waring
      // To conform the semantic of time_t,
      // it is not recommended use the number of days instead of seconds,
      // but it is only used here.
      time_t thisPeriod = now / kRollPerSeconds_; //* kRollPerSeconds;

      if (thisPeriod > startOfPeriod_) {
        rollFile();
      } else if (now - lastFlush_ > flushInterval_) {
        lastFlush_ = now;
        file_->flush();
      }
    }
  }
}

template<bool T>
void LogFile<T>::flush() noexcept {
  MutexGuardT<MutexPolicy> guard(lock_);
  file_->flush();
}

template<bool T>
void LogFile<T>::rollFile() {
  time_t now;  
  auto filename = getLogFileName(now);
  
  time_t newStartPeriod = now / kRollPerSeconds_;// * kRollPerSeconds_;
  if (now > lastRoll_) {
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = newStartPeriod;

    file_.reset(new AppendFile(filename));
  }
}

template<bool T>
std::string LogFile<T>::getLogFileName(time_t& now) {
  // Log file name format:
  // basename.timestamp.pid.hostname.log
  std::string filename;

  filename.reserve(basename_.size()+prefix_.size()+128);

  filename += prefix_.data();

  if (!prefix_.empty() && !prefix_.ends_with('/')) {
    filename += '/';
  }

  filename += basename_.data();

  char timebuf[32];
  struct tm tm;
  now = ::time(NULL);
  ::gmtime_r(&now, &tm);

  ::strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);

  filename += timebuf;
  filename += process::PidString().data();
  filename += ".";
  filename += process::Hostname().data();
  filename += ".log";

  return filename;
}

} // namespace kanon

#endif // KANON_LOGFILE_H
