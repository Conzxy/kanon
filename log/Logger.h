#ifndef KANON_LOGGER_H
#define KANON_LOGGER_H

#include "util/noncopyable.h"
#include "string/string-view.h"
#include "LogStream.h"
#include <functional>

namespace kanon {

extern __thread time_t t_lastSecond;
extern __thread char t_timebuf[64];

class Logger : noncopyable
{
	typedef StringView::size_type size_type;
	
	typedef std::function<void(char const*, size_t)> OutputCallback;
	typedef std::function<void()> FlushCallback;

public:
	enum class LogLevel: uint8_t
	{
		TRACE = 0,
		DEBUG,
		INFO,
		WARN,
		ERROR,
		FATAL,
		NUM_LOG_LEVEL,
	};
	
	// Since __FIEL__ is fullname, including all parent path
	struct SourceFile
	{
		// user-defined conversion char const* --> SourceFile
		// so not declared "explicit" keyword
		SourceFile(char const* fullname)
			: SourceFile(StringView(fullname))
		{ }
	
		StringView basename_;
	private:
		SourceFile(StringView fullname)
		{
			size_type pos;		
			while ((pos = fullname.find('/')) != StringView::npos)
			{
				fullname.remove_prefix(pos + 1);
			}
			
			basename_ = std::move(fullname);
		}
		
	};

	// TRACE DEBUG
	Logger(SourceFile basefile, size_t line, LogLevel level, char const* func);
	// INFO
	Logger(SourceFile basefile, size_t line);
	// To distinguish SYSXX and XX
	// use another parameter
	// 1) dummy paramter to overload
	// 2) different type parameter
	// Here use 2)
	//
	// WARN FATAL ERROR
	Logger(SourceFile basefile, size_t line, LogLevel level);
	// SYSERROR, SYSFATAL
	Logger(SourceFile basefile, size_t line, bool toAbort);
	
	// do output and flush
	~Logger();
	
	LogStream& stream() noexcept { return stream_; }

	static LogLevel logLevel() noexcept { return logLevel_; }
	static void setLogLevel(LogLevel logLevel) noexcept { logLevel_ = logLevel; }
	static OutputCallback outputCallback() noexcept { return outputCallback_; }
	static void setOutputCallback(OutputCallback output) noexcept { outputCallback_ = output; }
	static FlushCallback flushCallback() noexcept { return flushCallback_; }
	static void setFlushCallback(FlushCallback flush) noexcept { flushCallback_ = flush; }

private:
	StringView basename_;
	LogLevel curLogLevel_;
	//TimeStamp time_;
	size_t line_;

	LogStream stream_;
	
	void formatTime() noexcept;
	static char const* logLevelName[static_cast<size_t>(LogLevel::NUM_LOG_LEVEL)];
	static LogLevel logLevel_;
	static OutputCallback outputCallback_;
	static FlushCallback flushCallback_;

	// impl
	// errno is thread-local
	Logger(SourceFile const& file, LogLevel level, size_t line, int savedErrno);
};

char const* strerror_tl(int _errno);
void defaultFlush();
void defaultOutput(char const*, size_t);

#define LOG_TRACE \
	if (Logger::logLevel() <= Logger::LogLevel::TRACE) \
		Logger(__FILE__, __LINE__, Logger::LogLevel::TRACE, __func__).stream()

#define LOG_DEBUG \
	if (Logger::logLevel() <= Logger::LogLevel::DEBUG) \
		Logger(__FILE__, __LINE__, Logger::LogLevel::DEBUG).stream()
#define LOG_INFO \
	if (Logger::logLevel() <= Logger::LogLevel::INFO) \
		Logger(__FILE__, __LINE__).stream()

#define LOG_WARN \
	Logger(__FILE__, __LINE__, Logger::LogLevel::WARN).stream()

#define LOG_ERROR \
	Logger(__FILE__, __LINE__, Logger::LogLevel::ERROR).stream()

#define LOG_FATAL \
	Logger(__FILE__, __LINE__, Logger::LogLevel::FATAL).stream()

#define LOG_SYSERROR \
	Logger(__FILE__, __LINE__, false).stream()

#define LOG_SYSFATAL \
	Logger(__FILE__, __LINE__, true).stream()

} // namespace kanon

#endif // KANON_LOGGER_H
