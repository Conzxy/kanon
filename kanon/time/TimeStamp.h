#ifndef KANON_TIMESTAMP_H
#define KANON_TIMESTAMP_H

#include "kanon/util/operator_util.h"
#include "kanon/util/macro.h"

#include <time.h>
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

	bool valid() const KANON_NOEXCEPT
	{ return microseconds_ > 0; }
	
	int64_t microsecondsSinceEpoch() const KANON_NOEXCEPT
	{ return microseconds_; }
	
	int64_t seconds() const KANON_NOEXCEPT {
		return static_cast<int64_t>(microseconds_ / kMicrosecondsPerSeconds_);
	}
	
	void toInvalid() KANON_NOEXCEPT {
		microseconds_ = 0;
	}

	static TimeStamp fromUnixTime(time_t seconds, int64_t microseconds) KANON_NOEXCEPT
	{ return TimeStamp(static_cast<int64_t>(seconds * kMicrosecondsPerSeconds_ + microseconds)); }
	
	static TimeStamp fromUnixTime(time_t seconds) KANON_NOEXCEPT 
	{ return fromUnixTime(seconds, 0); }

	static TimeStamp now() KANON_NOEXCEPT;

	static constexpr int kMicrosecondsPerSeconds_ = 1000000;
private:
	int64_t microseconds_;
};

inline bool operator == (TimeStamp x, TimeStamp y) KANON_NOEXCEPT 
{ return x.microsecondsSinceEpoch() == y.microsecondsSinceEpoch(); }

inline bool operator < (TimeStamp x, TimeStamp y) KANON_NOEXCEPT
{ return x.microsecondsSinceEpoch() < y.microsecondsSinceEpoch(); }

inline double timeDifference(TimeStamp x, TimeStamp y) KANON_NOEXCEPT {
	int64_t diff = x.microsecondsSinceEpoch() - y.microsecondsSinceEpoch();
	return static_cast<double>(diff) / TimeStamp::kMicrosecondsPerSeconds_;
}

inline TimeStamp addTime(TimeStamp x, double seconds) KANON_NOEXCEPT {
	return TimeStamp(x.microsecondsSinceEpoch() + static_cast<int64_t>(seconds * TimeStamp::kMicrosecondsPerSeconds_));
}

} // namespace kanon

#endif // KANON_TIMESTAMP_H
