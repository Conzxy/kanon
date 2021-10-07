#ifndef KANON_TIMESTAMP_H
#define KANON_TIMESTAMP_H

#include <time.h>
#include "kanon/util/operator_util.h"
#include <stdint.h>
#include <string>

namespace kanon {

class TimeStamp
	: equal_comparable<TimeStamp>
	, less_than_comparable<TimeStamp>
{
public:
	TimeStamp() = default;
	explicit TimeStamp(int64_t microseconds)
		: microseconds_(microseconds)
	{ }

	~TimeStamp() = default;
	
	std::string toString() const;
	std::string toFormattedString(bool isShowMicroseconds) const;

	bool valid() const noexcept
	{ return microseconds_ > 0; }
	
	int64_t microsecondsSinceEpoch() const noexcept
	{ return microseconds_; }
	
	int64_t seconds() const noexcept {
		return static_cast<int64_t>(microseconds_ / kMicrosecondsPerSeconds_);
	}

	static TimeStamp fromUnixTime(time_t seconds, int64_t microseconds) noexcept
	{ return TimeStamp(static_cast<int64_t>(seconds * kMicrosecondsPerSeconds_ + microseconds)); }
	
	static TimeStamp fromUnixTime(time_t seconds) noexcept 
	{ return fromUnixTime(seconds, 0); }

	static TimeStamp now() noexcept;

	static constexpr int kMicrosecondsPerSeconds_ = 1000000;
private:
	int64_t microseconds_;
};

inline bool operator == (TimeStamp x, TimeStamp y) noexcept 
{ return x.microsecondsSinceEpoch() == y.microsecondsSinceEpoch(); }

inline bool operator < (TimeStamp x, TimeStamp y) noexcept
{ return x.microsecondsSinceEpoch() < y.microsecondsSinceEpoch(); }

inline double timeDifference(TimeStamp x, TimeStamp y) noexcept {
	int64_t diff = x.microsecondsSinceEpoch() - y.microsecondsSinceEpoch();
	return static_cast<double>(diff) / TimeStamp::kMicrosecondsPerSeconds_;
}

inline TimeStamp addTime(TimeStamp x, double seconds) noexcept {
	return TimeStamp(x.microsecondsSinceEpoch() + static_cast<int64_t>(seconds * TimeStamp::kMicrosecondsPerSeconds_));
}

} // namespace kanon

#endif // KANON_TIMESTAMP_H
