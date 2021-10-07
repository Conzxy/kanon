#include "Logger.h"
#include "TerminalColor.h"
#include "kanon/thread/current-thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

namespace kanon {

__thread time_t t_lastSecond = 0;
__thread char t_timebuf[64] = { 0 };

char const* Logger::logLevelName[static_cast<size_t>(Logger::LogLevel::NUM_LOG_LEVEL)] = {
	"TRACE",
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	"FATAL"
};

static char const* g_logLevelColor[] = {
	CYAN,
	BLUE,
	GREEN,
	YELLOW,
	RED,
	L_RED
};

Logger::LogLevel initLogLevel()
{
	if (::getenv("KANON_TRACE"))
		return Logger::LogLevel::TRACE;
	else if (::getenv("KANON_DEBUG"))
		return Logger::LogLevel::DEBUG;
	else 
		return Logger::LogLevel::INFO;
}

Logger::LogLevel Logger::logLevel_ = initLogLevel();

void defaultOutput(char const* data, size_t num)
{
	::fwrite(data, 1, num, stdout);
}

void defaultFlush()
{
	::fflush(stdout);
}

Logger::OutputCallback Logger::outputCallback_ = &defaultOutput;
Logger::FlushCallback Logger::flushCallback_ = &defaultFlush;

#define ERRNO_BUFFER_SIZE 1024
#define TIME_BUFFER_SIZE 64

__thread char t_errorBuf[ERRNO_BUFFER_SIZE];
__thread char t_timeBuf[TIME_BUFFER_SIZE];
__thread int t_lastSec;

char const* strerror_tl(int savedErrno) {
	return ::strerror_r(savedErrno, t_errorBuf, ERRNO_BUFFER_SIZE);
}

Logger::Logger(SourceFile basefile, size_t line, LogLevel level, char const* func)
	: Logger(basefile, level, line, 0)
{ 
	stream_ << func << "() ";
}

Logger::Logger(SourceFile basefile, size_t line, LogLevel level)
	: Logger(basefile, level, line, 0)
{ }

Logger::Logger(SourceFile basefile, size_t line)
	: Logger(basefile, LogLevel::INFO, line, 0)
{ }

Logger::Logger(SourceFile basefile, size_t line, bool toAbort)
	: Logger(basefile, toAbort ? LogLevel::FATAL : LogLevel::ERROR, line, errno)
{ }

Logger::Logger(SourceFile const& file, LogLevel level, size_t line, int savedErrno)
	: basename_(file.basename_)
	, curLogLevel_(level)
	, line_(line)
{
	formatTime();
	stream_ << CurrentThread::t_tid << " ";
	stream_ << CurrentThread::t_name << " ";
	stream_ << g_logLevelColor[static_cast<size_t>(level)] 
			<< "[" << logLevelName[static_cast<size_t>(level)] << "]"
			<< NONE << " ";

	if (savedErrno) {
		snprintf(t_errorBuf, ERRNO_BUFFER_SIZE, "errno: %d, errmsg: %s",
				savedErrno, strerror_tl(savedErrno));
		stream_ << t_errorBuf << " ";
	}
}

Logger::~Logger() {
	stream_ << " - " << basename_ << ":" << line_ << "\n";
	outputCallback_(stream_.data(), stream_.size());
	
	if (curLogLevel_ == LogLevel::FATAL) {
		flushCallback_();
		abort();
	}
}

void Logger::formatTime() noexcept {
	// don't consider time zone
	struct timeval tv;
	::gettimeofday(&tv, NULL);
	
	time_t nowSecond = tv.tv_sec;
	int microsecond= tv.tv_usec;

	if (nowSecond != t_lastSecond) {
		// cache second
		t_lastSecond = nowSecond;
		struct tm tm;
		::gmtime_r(&t_lastSecond, &tm);

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
