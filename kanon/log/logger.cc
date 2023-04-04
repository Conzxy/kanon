#include "kanon/log/logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "kanon/thread/current_thread.h"
#include "kanon/util/time.h"
#include "kanon/log/terminal_color.h"

#ifdef KANON_ON_WIN
#  include <windows.h>
#  include <winbase.h>
#endif

namespace kanon {

static KANON_TLS time_t t_lastSecond = 0;
static KANON_TLS char t_timebuf[64] = { 0 };

bool g_kanon_log = true;
bool g_all_log = true;

bool Logger::need_color_ = true;

char const
    *Logger::s_log_level_names_[Logger::LogLevel::KANON_LL_NUM_LOG_LEVEL] = {
      "TRACE", "DEBUG", "INFO",      "WARN",
      "ERROR", "FATAL", "SYS_ERROR", "SYS_FATAL",
    };

static char const *g_logLevelColor[] = { CYAN, BLUE,  GREEN, YELLOW,
                                         RED,  L_RED, RED,   L_RED };

static Logger::LogLevel initLogLevel() KANON_NOEXCEPT
{
  if (!g_all_log) return Logger::KANON_LL_OFF;
  auto log_enable = ::getenv("KANON_LOG_ENABLE");
  if (log_enable && !StrCaseCompare(log_enable, "0")) {
    g_kanon_log = 0;
  }

  auto log_level = ::getenv("KANON_LOG");
  if (!log_level) return Logger::LogLevel::KANON_LL_INFO;

  if (!StrCaseCompare(log_level, "trace")) {
    return Logger::LogLevel::KANON_LL_TRACE;
  }
  if (!StrCaseCompare(log_level, "debug")) {
    return Logger::LogLevel::KANON_LL_DEBUG;
  }
  if (!StrCaseCompare(log_level, "warn") ||
      !StrCaseCompare(log_level, "warning")) {
    return Logger::LogLevel::KANON_LL_WARN;
  }
  if (!StrCaseCompare(log_level, "error")) {
    return Logger::LogLevel::KANON_LL_ERROR;
  }
  if (!StrCaseCompare(log_level, "fatal")) {
    return Logger::LogLevel::KANON_LL_FATAL;
  }
  if (!StrCaseCompare(log_level, "off")) {
    return Logger::LogLevel::KANON_LL_OFF;
  }
  return Logger::LogLevel::KANON_LL_INFO;
}

Logger::LogLevel Logger::log_level_ = initLogLevel();

void DefaultOutput(char const *data, size_t num)
{
  ::fwrite(data, 1, num, stdout);
}

void DefaultFlush()
{
  ::fflush(stdout);
}

Logger::OutputCallback Logger::output_callback_ = &DefaultOutput;
Logger::FlushCallback Logger::flush_callback_ = &DefaultFlush;

#define ERRNO_BUFFER_SIZE 1124
#define TIME_BUFFER_SIZE  64

KANON_TLS char t_errorBuf[ERRNO_BUFFER_SIZE];
KANON_TLS char t_lastErrorBuf[ERRNO_BUFFER_SIZE];

// tl = thread local
char const *strerror_tl(int savedErrno)
{
#ifdef KANON_ON_UNIX
  auto ret = ::strerror_r(savedErrno, t_errorBuf, ERRNO_BUFFER_SIZE);
#else
  auto ret = ::strerror_s(t_errorBuf, ERRNO_BUFFER_SIZE, savedErrno);
#endif
  KANON_UNUSED(ret);
  return t_errorBuf;
}

#ifdef KANON_ON_WIN
char const *kanon::win_last_strerror(int win_errno)
{
  ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, win_errno, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  t_lastErrorBuf, ERRNO_BUFFER_SIZE, NULL);
  return t_lastErrorBuf;
}
#endif

Logger::Logger(SourceFile file, size_t line, LogLevel level)
  : basename_(file.basename_)
  , cur_log_level_(level)
  , line_(line)
{
  FormatTime();
  CurrentThread::tid();
  stream_ << StringView(CurrentThread::t_tidString,
                        (StringView::size_type)CurrentThread::t_tidLength)
          << " ";
  if (CurrentThread::t_name)
    stream_ << CurrentThread::t_name << " ";
  else
    stream_ << "Unnamed ";

  if (need_color_) {
    stream_ << g_logLevelColor[level] << "[" << s_log_level_names_[level] << "]"
            << NONE << " ";
  } else {
    stream_ << "[" << s_log_level_names_[level] << "] ";
  }
}

Logger::Logger(SourceFile basefile, size_t line, LogLevel level, bool is_sys)
  : Logger(basefile, line, level)
{
  // dummy parameter
  KANON_UNUSED(is_sys);
  char error_buf[4096];
#ifdef KANON_ON_UNIX
  auto savedErrno = errno;
  if (savedErrno) {
    ::snprintf(error_buf, ERRNO_BUFFER_SIZE, "errno: %d, errmsg: %s",
               savedErrno, strerror_tl(savedErrno));
  }
#elif defined(KANON_ON_WIN)
  auto savedErrno = ::WSAGetLastError();
  if (savedErrno) {
    ::snprintf(error_buf, ERRNO_BUFFER_SIZE, "errno: 0x%d, errmsg: %s",
               savedErrno, win_last_strerror(savedErrno));
  }
#endif
  stream_ << error_buf << " ";
  errno = savedErrno;
}

Logger::~Logger() KANON_NOEXCEPT
{
  stream_ << " - " << basename_ << ":" << line_ << "\n";
  output_callback_(stream_.data(), stream_.size());

  if (cur_log_level_ == KANON_LL_FATAL || cur_log_level_ == KANON_LL_SYS_FATAL)
  {
    flush_callback_();
    abort();
  }
}

void Logger::FormatTime() KANON_NOEXCEPT
{
  struct timeval tv;
  kanon::GetTimeOfDay(&tv, NULL);

  time_t nowSecond = tv.tv_sec;
  auto microsecond = (int)tv.tv_usec;

  // Don't use TimeStamp::Now()
  // Cache second to decrease the count of calling sys API.
  // t_timebuf also cache the result of API, and only update the microsecond
  // part when second is not changed.
  if (nowSecond != t_lastSecond) {
    // cache second
    t_lastSecond = nowSecond;
    struct tm tm;
    // ::gmtime_r(&t_lastSecond, &tm);
#ifdef KANON_ON_UNIX
    ::localtime_r(&t_lastSecond, &tm);
#else
    // FIXME win specific?
    ::localtime_s(&tm, &t_lastSecond);
#endif

    ::snprintf(t_timebuf, sizeof t_timebuf, "%04d%02d%02d:%02d%02d%02d.%06d",
               tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
               tm.tm_min, tm.tm_sec, microsecond);

    assert(strlen(t_timebuf) == 22);
    stream_ << t_timebuf << " ";
  } else {
    stream_ << StringView(t_timebuf, 16);
    char us_buf[32];
    auto ret = ::snprintf(us_buf, sizeof us_buf, "%06d", microsecond);
    if (ret > 0) {
      stream_ << StringView{ us_buf, (StringView::size_type)ret };
    }
    stream_ << ' ';
  }
}

} // namespace kanon
