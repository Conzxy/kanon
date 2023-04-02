#ifndef KANON_LOGGER_H
#define KANON_LOGGER_H

#include <functional>

#include "kanon/string/fmt_stream.h"
#include "kanon/string/string_view.h"
#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"

#include "kanon/log/log_stream.h"

namespace kanon {

// __thread time_t t_lastSecond;
// __thread char t_timebuf[64];

KANON_CORE_API extern bool g_kanon_log;
KANON_CORE_API extern bool g_all_log;

/**
 * Enable/Disable the logging of kanon library
 */
KANON_INLINE void SetKanonLog(bool val) KANON_NOEXCEPT
{
  g_kanon_log = val;
}

/**
 * Control the logging of logger(i.e. All logs output by kanon)
 */
KANON_INLINE void EnableAllLog(bool value) KANON_NOEXCEPT
{
  g_all_log = value;
}

/**
 * \brief Format the log message to specified device
 *
 * The log format:
 * timestamp thread_id thread_name [log_level] (errno errmsg) (function_name)
 * (user-provided message) - filename:line_no
 *
 *  - errno errmsg only used for SYSERROR, SYSFATAL
 *  - function_name only used for TRACE, DEBUG
 *  - user-provided message throught user call LOG_XXX
 *
 * Because the user-provided message in the middle
 * we use RAII property to construct entire log message
 *
 * \note
 *   The default output device is stdout(i.e. terminal).
 *   Change this by modify append and flush callback
 * \see
 *   AsyncLog, LogFile
 */
class Logger : noncopyable {
  using size_type = StringView::size_type;
  using OutputCallback = std::function<void(char const *, size_t)>;
  using FlushCallback = std::function<void()>;

 public:
  /**
   * Default log level is INFO if user don't specify the environment variable
   * The highest log level can be specified is INFO
   * This indicates the WARN, (SYS)ERROR, (SYS)FATAL is don't effect by user.
   * Obviously, if current log level > LOG_XXX, can't log and message
   * e.g. current log level is DEBUG, user call LOG_TRACE is no effect
   * i.e. Log condiftion is current log level <= XXX
   */
  enum LogLevel {
    KANON_LL_TRACE = 0,
    KANON_LL_DEBUG = 1,
    KANON_LL_INFO = 2,
    KANON_LL_WARN = 3,
    KANON_LL_ERROR = (1 << 2),
    KANON_LL_SYS_ERROR = KANON_LL_ERROR + 1,
    KANON_LL_FATAL = (1 << 3),
    KANON_LL_SYS_FATAL = KANON_LL_FATAL + 1,
    KANON_LL_NUM_LOG_LEVEL = 8,
    KANON_LL_OFF = (1 << 4),
  };

  // Since __FIEL__ is fullname, including all parent path
  struct KANON_CORE_NO_API SourceFile {
    // user-defined conversion char const* --> SourceFile
    // so dose not declared "explicit" keyword
    //
    // This is not a good habit to use template argument deduction
    // to get the length of string literal.
    //
    // template <unsigned N>
    // SourceFile(char const (&fullname)[N])
    //  : SourceFile(MakeStringView(fullname))
    //{
    //}

    SourceFile(char const *fullname)
      : SourceFile(StringView(fullname))
    {
    }

    StringView basename_;

   private:
    SourceFile(StringView fullname)
    {
      const size_type pos = fullname.rfind('/');

      if (pos != StringView::npos) {
        basename_ = fullname.substr(fullname.rfind('/') + 1);
      } else {
        basename_ = fullname;
      }
    }
  };

  // TRACE DEBUG
  // Need put function name to let user know the call track
  Logger(SourceFile basefile, size_t line, LogLevel level, char const *func)
    : Logger(basefile, line, level)
  {
    stream_ << func << "() ";
  }

  // INFO WARN
  // This is also a forward constructor of others
  KANON_CORE_API Logger(SourceFile basefile, size_t line, LogLevel level);

  // ERROR FATAL SYS_ERROR SYS_FATAL
  // Because I don't let INFO and WARN also check whether this is a
  // SYS_ERROR/SYS_FATAL, split them to two parts
  KANON_CORE_API Logger(SourceFile basefile, size_t line, LogLevel level,
                        bool is_sys);

  // Do output and flush
  KANON_CORE_API ~Logger() KANON_NOEXCEPT;

  KANON_INLINE LogStream &stream() KANON_NOEXCEPT
  {
    return stream_;
  }

  KANON_INLINE static void SetColor(bool c) KANON_NOEXCEPT
  {
    need_color_ = c;
  }
  KANON_INLINE static LogLevel GetLogLevel() KANON_NOEXCEPT
  {
    return log_level_;
  }
  KANON_INLINE static void SetLogLevel(LogLevel level) KANON_NOEXCEPT
  {
    log_level_ = level;
  }
  KANON_INLINE static OutputCallback GetOutputCallback() KANON_NOEXCEPT
  {
    return output_callback_;
  }

  static void SetOutputCallback(OutputCallback output) KANON_NOEXCEPT
  {
    output_callback_ = output;
  }
  static FlushCallback GetFlushCallback() KANON_NOEXCEPT
  {
    return flush_callback_;
  }
  static void SetFlushCallback(FlushCallback flush) KANON_NOEXCEPT
  {
    flush_callback_ = flush;
  }

 private:
  void FormatTime() KANON_NOEXCEPT;

  StringView basename_; /** Filename(may including slash) */
  LogLevel cur_log_level_;
  size_t line_; /** Line number */

  LogStream stream_;

  static char const *s_log_level_names_[KANON_LL_NUM_LOG_LEVEL];
  /**
   * Current Log Level, initial value is INFO
   * You can define environment variable to specify WARN or TRACE
   */
  KANON_CORE_API static LogLevel log_level_;
  KANON_CORE_API static bool need_color_;
  KANON_CORE_API static OutputCallback output_callback_;
  KANON_CORE_API static FlushCallback flush_callback_;
};

KANON_CORE_API char const *strerror_tl(int _errno);
#ifdef KANON_ON_WIN
KANON_CORE_API char const *win_last_strerror(int win_errno);
#endif

KANON_CORE_API void DefaultFlush();
KANON_CORE_API void DefaultOutput(char const *, size_t);

#ifdef LOG_TRACE
#  undef LOG_TRACE
#endif

#ifdef LOG_DEBUG
#  undef LOG_DEBUG
#endif

#ifdef LOG_INFO
#  undef LOG_INFO
#endif

#ifdef LOG_WARN
#  undef LOG_WARN
#endif

#ifdef LOG_ERROR
#  undef LOG_ERROR
#endif

#ifdef LOG_FATAL
#  undef LOG_FATAL
#endif

#ifdef LOG_SYSERROR
#  undef LOG_SYSERROR
#endif

#ifdef LOG_SYSFATAL
#  undef LOG_SYSFATAL
#endif

#define LOG_TRACE                                                              \
  if (kanon::Logger::GetLogLevel() <= kanon::Logger::KANON_LL_TRACE)           \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_TRACE, __func__)   \
      .stream()

#define LOG_TRACE_KANON                                                        \
  if (g_kanon_log) LOG_TRACE

#define LOG_DEBUG                                                              \
  if (kanon::Logger::GetLogLevel() <= kanon::Logger::KANON_LL_DEBUG)           \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_DEBUG, __func__)   \
      .stream()

#define LOG_DEBUG_KANON                                                        \
  if (g_kanon_log) LOG_DEBUG

#define LOG_INFO                                                               \
  if (kanon::Logger::GetLogLevel() <= kanon::Logger::KANON_LL_INFO)            \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_INFO).stream()

#define LOG_INFO_KANON                                                         \
  if (g_kanon_log) LOG_INFO

#define LOG_WARN                                                               \
  if (kanon::Logger::GetLogLevel() <= kanon::Logger::KANON_LL_WARN)            \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_WARN).stream()

#define LOG_WARN_KANON                                                         \
  if (g_kanon_log) LOG_WARN

#define LOG_ERROR                                                              \
  if (kanon::Logger::GetLogLevel() <= kanon::Logger::KANON_LL_ERROR)           \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_ERROR).stream()

#define LOG_ERROR_KANON                                                        \
  if (g_kanon_log) LOG_ERROR

#define LOG_FATAL                                                              \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_FATAL).stream()

#define LOG_SYSERROR                                                           \
  if (kanon::Logger::GetLogLevel() <= kanon::Logger::KANON_LL_SYS_ERROR)       \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_SYS_ERROR, true)   \
      .stream()

#define LOG_SYSERROR_KANON                                                     \
  if (g_kanon_log) LOG_SYSERROR

#define LOG_SYSFATAL                                                           \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_SYS_FATAL, true)   \
      .stream()

#define FMT_LOG_TRACE(fmt, ...)                                                \
  LOG_TRACE << kanon::LogFmtStream(fmt, __VA_ARGS__).ToStringView()

#define FMT_LOG_DEBUG(fmt, ...)                                                \
  LOG_DEBUG << kanon::LogFmtStream(fmt, __VA_ARGS__).ToStringView()

#define FMT_LOG_INFO(fmt, ...)                                                 \
  LOG_INFO << kanon::LogFmtStream(fmt, __VA_ARGS__).ToStringView()

#define FMT_LOG_WARN(fmt, ...)                                                 \
  LOG_WARN << kanon::LogFmtStream(fmt, __VA_ARGS__).ToStringView()

#define FMT_LOG_ERROR(fmt, ...)                                                \
  LOG_ERROR << kanon::LogFmtStream(fmt, __VA_ARGS__).ToStringView()

#define FMT_LOG_FATAL(fmt, ...)                                                \
  LOG_FATAL << kanon::LogFmtStream(fmt, __VA_ARGS__).ToStringView()

#define FMT_LOG_SYSERROR(fmt, ...)                                             \
  LOG_SYSERROR << kanon::LogFmtStream(fmt, __VA_ARGS__).ToStringView()

#define FMT_LOG_SYSFATAL(fmt, ...)                                             \
  LOG_SYSFATAL << kanon::LogFmtStream(fmt, __VA_ARGS__).ToStringView()

} // namespace kanon

#endif // KANON_LOGGER_H
