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

extern bool g_kanon_log;
extern bool g_all_log;

/**
 * Enable/Disable the logging of kanon library
 */
inline void SetKanonLog(bool val) noexcept { g_kanon_log = val; }

/**
 * Control the logging of logger(i.e. All logs output by kanon)
 */
inline void EnableAllLog(bool value) noexcept { g_all_log = value; }

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
    KANON_LL_DEBUG,
    KANON_LL_INFO,
    KANON_LL_WARN,
    KANON_LL_ERROR,
    KANON_LL_FATAL,
    KANON_LL_SYS_ERROR,
    KANON_LL_SYS_FATAL,
    KANON_LL_NUM_LOG_LEVEL,
  };

  // Since __FIEL__ is fullname, including all parent path
  struct SourceFile {
    // user-defined conversion char const* --> SourceFile
    // so dose not declared "explicit" keyword
    template <unsigned N>
    SourceFile(char const (&fullname)[N])
      : SourceFile(MakeStringView(fullname))
    {
    }

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
  Logger(SourceFile basefile, size_t line, LogLevel level);

  // ERROR FATAL SYS_ERROR SYS_FATAL
  // Because I don't let INFO and WARN also check whether this is a
  // SYS_ERROR/SYS_FATAL, split them to two parts
  Logger(SourceFile basefile, size_t line, LogLevel level, bool is_sys);

  // Do output and flush
  ~Logger() noexcept;

  LogStream &stream() noexcept { return stream_; }

  static void SetColor(bool c) noexcept { need_color_ = c; }
  static LogLevel GetLogLevel() noexcept { return log_level_; }
  static void SetLogLevel(LogLevel level) noexcept { log_level_ = level; }
  static OutputCallback GetOutputCallback() noexcept
  {
    return output_callback_;
  }
  static void SetOutputCallback(OutputCallback output) noexcept
  {
    output_callback_ = output;
  }
  static FlushCallback GetFlushCallback() noexcept { return flush_callback_; }
  static void SetFlushCallback(FlushCallback flush) noexcept
  {
    flush_callback_ = flush;
  }

 private:
  StringView basename_; /** Filename(may including slash) */
  LogLevel cur_log_level_;
  size_t line_; /** Line number */

  LogStream stream_;

  void FormatTime() noexcept;

  static char const *s_log_level_names_[KANON_LL_NUM_LOG_LEVEL];
  /**
   * Current Log Level, initial value is INFO
   * You can define environment variable to specify WARN or TRACE
   */
  static LogLevel log_level_;
  static bool need_color_;
  static OutputCallback output_callback_;
  static FlushCallback flush_callback_;
};

char const *strerror_tl(int _errno);
void DefaultFlush();
void DefaultOutput(char const *, size_t);

#define LOG_TRACE                                                              \
  if (kanon::Logger::GetLogLevel() <= kanon::Logger::KANON_LL_TRACE &&         \
      kanon::g_all_log)                                                        \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_TRACE, __func__)   \
      .stream()

#define LOG_TRACE_KANON                                                        \
  if (g_kanon_log) LOG_TRACE

#define LOG_DEBUG                                                              \
  if (kanon::Logger::GetLogLevel() <= kanon::Logger::KANON_LL_DEBUG &&         \
      kanon::g_all_log)                                                        \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_DEBUG, __func__)   \
      .stream()

#define LOG_DEBUG_KANON                                                        \
  if (g_kanon_log) LOG_DEBUG

#define LOG_INFO                                                               \
  if (kanon::Logger::GetLogLevel() <= kanon::Logger::KANON_LL_INFO &&          \
      kanon::g_all_log)                                                        \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_INFO).stream()

#define LOG_WARN                                                               \
  if (kanon::g_all_log)                                                        \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_WARN).stream()

#define LOG_ERROR                                                              \
  if (kanon::g_all_log)                                                        \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_ERROR).stream()

#define LOG_FATAL                                                              \
  if (kanon::g_all_log)                                                        \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_FATAL).stream()

#define LOG_SYSERROR                                                           \
  if (kanon::g_all_log)                                                        \
  kanon::Logger(__FILE__, __LINE__, kanon::Logger::KANON_LL_SYS_ERROR, true)   \
      .stream()

#define LOG_SYSFATAL                                                           \
  if (kanon::g_all_log)                                                        \
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
