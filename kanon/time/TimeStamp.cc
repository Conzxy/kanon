#include "TimeStamp.h"
#include <sys/time.h>
#include <inttypes.h>

namespace kanon {
	
TimeStamp TimeStamp::now() noexcept {
	struct timeval time;
	::gettimeofday(&time, NULL);
	return TimeStamp(static_cast<int64_t>(time.tv_sec * kMicrosecondsPerSeconds_ + time.tv_usec));
}

std::string TimeStamp::toString() const {
	char buf[64] {0};
	
	int64_t seconds = microseconds_ / kMicrosecondsPerSeconds_;
	int microseconds = static_cast<int>(microseconds_ % kMicrosecondsPerSeconds_);
	snprintf(buf, sizeof buf, "%" PRId64 ".6%d", seconds, microseconds);

	return buf;
}

std::string TimeStamp::toFormattedStirng(bool isShowMicroseconds) const {
	char buf[64] {0};

	auto seconds = static_cast<time_t>(microseconds_ / kMicrosecondsPerSeconds_);
	struct tm tm;
	gmtime_r(&seconds, &tm);
	
	if (isShowMicroseconds) {
		int microseconds = static_cast<int>(microseconds_ % kMicrosecondsPerSeconds_);
		
		snprintf(buf, sizeof buf, "%4d%02d%02d %02d:%02d:%02d.%06d",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec,
				microseconds);
	} else {
		snprintf(buf, sizeof buf, "%04d%02d%02d %02d:%02d:%02d",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec);
	}

	return buf;
}

} // namespace kanon
