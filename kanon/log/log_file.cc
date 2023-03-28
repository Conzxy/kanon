#include "log_file.h"

namespace kanon {

template <bool T>
LogFile<T>::LogFile(StringView basename, size_t roll_size, StringView prefix,
                    size_t log_file_num, size_t roll_interval,
                    size_t flush_interval)
  : basename_(basename.ToString())
  , roll_size_(roll_size)
  , start_of_period_(0)
  , roll_interval_(roll_interval)
  , last_roll_(0)
  , last_flush_(0)
  , flush_interval_(static_cast<time_t>(flush_interval))
  , lock_()
  , prefix_(prefix.ToString())
  , log_file_num_(log_file_num)
  , remove_thr_("RemoveLogFile")
  , remove_cond_(remove_mtx_)
  , quit_remove_thr_(false)
{
  assert(basename.find('/') == StringView::npos);
  Logger::SetColor(false);
  RollFile();

  if (log_file_num_ != UINT_MAX) {
    remove_thr_.StartRun([this]() {
      while (!quit_remove_thr_) {
        {
          MutexGuard guard(remove_mtx_);
          // There is one consumer only, don't use while is also ok
          if (log_files_dup_.size() < log_file_num_ - 1) {
            remove_cond_.Wait();
          }
        }

        for (auto log_file : log_files_dup_) {
          ::remove(log_file.c_str());
        }
        log_files_dup_.clear();
      }
    });
  }
}

template <bool T>
inline LogFile<T>::~LogFile() noexcept
{
  if (log_file_num_ != UINT_MAX) {
    quit_remove_thr_ = true;
    remove_cond_.Notify();

    remove_thr_.Join();
  }
}

template <bool T>
void LogFile<T>::Append(char const *data, size_t num) noexcept
{
  MutexGuardT<MutexPolicy> guard(lock_);

  file_->Append(data, num);

  if (file_->writtenBytes() > roll_size_) {
    RollFile();
  } else {
    // second precision
    auto now = ::time(NULL);

    // \waring
    // To conform the semantic of time_t,
    // it is not recommended to use the number of days instead of seconds,
    // but it is only used here.
    time_t this_period = now / roll_interval_; //* roll_interval;

    // Over the roll_interval_, roll new file
    if (this_period > start_of_period_) {
      RollFile();
    } else if (now - last_flush_ > flush_interval_) {
      last_flush_ = now;
      file_->Flush();
    }
  }

  if (log_files_.size() >= log_file_num_) {
    auto last_file = std::move(log_files_.back());
    log_files_.pop_back();

    log_files_dup_.swap(log_files_);
    log_files_.emplace_back(std::move(last_file));

    MutexGuard guard(remove_mtx_);
    remove_cond_.Notify();
  }
}

template <bool T>
void LogFile<T>::Flush() noexcept
{
  MutexGuardT<MutexPolicy> guard(lock_);
  file_->Flush();
}

template <bool T>
void LogFile<T>::RollFile()
{
  time_t now = ::time(NULL);
  time_t new_start_period = now / roll_interval_;

  if (now > last_roll_) {
    auto filename = GetLogFilename(now);
    last_roll_ = now;
    last_flush_ = now;
    start_of_period_ = new_start_period;

    file_.reset(new AppendFile(filename));
    log_files_.emplace_back(filename);
  }
}

template <bool T>
std::string LogFile<T>::GetLogFilename(time_t &now)
{
  // Log file name format:
  // basename.timestamp.pid.hostname.log
  std::string filename;

  filename.reserve(basename_.size() + prefix_.size() + 128);

  filename += prefix_.data();

  if (!prefix_.empty() && !(prefix_.back() == '/')) {
    filename += '/';
  }

  filename += basename_.data();

  char timebuf[32];
  ::tzset();

  // FIXME Don't call localtime_r() here?
#if 0
  struct tm tm;
  ::localtime_r(&now, &tm);
#endif
  auto tm = localtime(&now);
  ::strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", tm);

  filename += timebuf;
  filename += process::PidString().data();
  filename += ".";
  filename += process::Hostname().data();
  filename += ".log";

  return filename;
}

template class LogFile<true>;
template class LogFile<>;

} // namespace kanon