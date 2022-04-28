#include "kanon/log/logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#include "kanon/thread/current_thread.h"

#include "kanon/log/terminal_color.h"

namespace kanon {

static __thread time_t t_lastSecond = 0;
static __thread char t_timebuf[64] = { 0 };

bool Logger::need_color_ = true;

char const* Logger::s_log_level_names_[Logger::LogLevel::NUM_LOG_LEVEL] = {
  "TRACE",
  "DEBUG",
  "INFO",
  "WARN",
  "ERROR",
  "FATAL",
  "SYS_ERROR",
  "SYS_FAYAL",
};

static char const* g_logLevelColor[] = {
  CYAN,
  BLUE,
  GREEN,
  YELLOW,
  RED,
  L_RED,
  RED,
  L_RED
};

static Logger::LogLevel initLogLevel() noexcept
{
  if (::getenv("KANON_TRACE"))
    return Logger::LogLevel::TRACE;
  else if (::getenv("KANON_DEBUG"))
    return Logger::LogLevel::DEBUG;
  else 
    return Logger::LogLevel::INFO;
}

Logger::LogLevel Logger::log_level_ = initLogLevel();

void DefaultOutput(char const* data, size_t num)
{
  ::fwrite(data, 1, num, stdout);
}

void DefaultFlush()
{
  ::fflush(stdout);
}

Logger::OutputCallback Logger::output_callback_ = &DefaultOutput;
Logger::FlushCallback Logger::flush_callback_ = &DefaultFlush;

#define ERRNO_BUFFER_SIZE 1024
#define TIME_BUFFER_SIZE 64

__thread char t_errorBuf[ERRNO_BUFFER_SIZE];

// tl = thread local
char const* strerror_tl(int savedErrno) {
  return ::strerror_r(savedErrno, t_errorBuf, ERRNO_BUFFER_SIZE);
}

Logger::Logger(SourceFile file, size_t line, LogLevel level)
  : basename_(file.basename_)
  , cur_log_level_(level)
  , line_(line)
{
  FormatTime();
  stream_ << CurrentThread::t_tid << " ";
  stream_ << CurrentThread::t_name << " ";
  if (need_color_) {
    stream_ << g_logLevelColor[level] 
        << "[" << s_log_level_names_[level] << "]"
        << NONE << " ";
  }
  else {
    stream_ << "[" << s_log_level_names_[level] << "] ";
  }

}

Logger::Logger(SourceFile basefile, size_t line, LogLevel level, bool is_sys) 
  : Logger(basefile, line, level) {
  if (is_sys) {
    auto savedErrno = errno;
    if (savedErrno) {
      ::snprintf(t_errorBuf, ERRNO_BUFFER_SIZE, "errno: %d, errmsg: %s",
          savedErrno, strerror_tl(savedErrno));
      stream_ << t_errorBuf << " ";
    }
    errno = savedErrno;
  }
}

Logger::~Logger() noexcept 
{
  stream_ << " - " << basename_ << ":" << line_ << "\n";
  output_callback_(stream_.data(), stream_.size());
  
  if (cur_log_level_ == FATAL || cur_log_level_ == SYS_FATAL) {
    flush_callback_();
    abort();
  }
}

void Logger::FormatTime() noexcept {
  struct timeval tv;
  ::gettimeofday(&tv, NULL);
  
  time_t nowSecond = tv.tv_sec;
  int microsecond= tv.tv_usec;

  // Don't use TimeStamp::Now()
  // Cache second to decrease the count of calling sys API.
  // t_timebuf also cache the result of API, and only update the microsecond part
  // when second is not changed.
  if (nowSecond != t_lastSecond) {
    // cache second
    t_lastSecond = nowSecond;
    struct tm tm;
    // ::gmtime_r(&t_lastSecond, &tm);
    ::localtime_r(&t_lastSecond, &tm);

    ::snprintf(t_timebuf, sizeof t_timebuf, "%04d%02d%02d:%02d%02d%02d.%06d",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec,
        microsecond);
  
    assert(strlen(t_timebuf) == 22);
    stream_ << t_timebuf << " ";
  } else {
    Fmt us("%06d", microsecond);
    stream_ << StringView(t_timebuf, 16) << us << " ";
  }
}

} // namespace kanon