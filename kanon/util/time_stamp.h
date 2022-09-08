#ifndef KANON_TIMESTAMP_H
#define KANON_TIMESTAMP_H

#include <time.h>
#include <stdint.h>
#include <string>

#include "kanon/util/operator_util.h"
#include "kanon/util/macro.h"

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
  
  std::string ToString() const;
  std::string ToFormattedString(bool isShowMicroseconds = true) const;

  bool IsValid() const noexcept
  { return microseconds_ > 0; }
  
  int64_t GetMicrosecondsSinceEpoch() const noexcept
  { return microseconds_; }
  
  int64_t GetSeconds() const noexcept {
    return static_cast<int64_t>(microseconds_ / kMicrosecondsPerSeconds_);
  }
  
  void ToInvalid() noexcept {
    microseconds_ = 0;
  }

  static TimeStamp FromUnixTime(time_t seconds, int64_t microseconds) noexcept
  { return TimeStamp(static_cast<int64_t>(seconds * kMicrosecondsPerSeconds_ + microseconds)); }
  
  static TimeStamp FromUnixTime(time_t seconds) noexcept 
  { return FromUnixTime(seconds, 0); }

  static TimeStamp Now() noexcept;

  static constexpr int kMicrosecondsPerSeconds_ = 1000000;
private:
  int64_t microseconds_;
};

inline bool operator == (TimeStamp x, TimeStamp y) noexcept 
{ return x.GetMicrosecondsSinceEpoch() == y.GetMicrosecondsSinceEpoch(); }

inline bool operator < (TimeStamp x, TimeStamp y) noexcept
{ return x.GetMicrosecondsSinceEpoch() < y.GetMicrosecondsSinceEpoch(); }

inline double TimeDifference(TimeStamp x, TimeStamp y) noexcept {
  int64_t diff = x.GetMicrosecondsSinceEpoch() - y.GetMicrosecondsSinceEpoch();
  return static_cast<double>(diff) / TimeStamp::kMicrosecondsPerSeconds_;
}

inline TimeStamp AddTime(TimeStamp x, double seconds) noexcept {
  return TimeStamp(x.GetMicrosecondsSinceEpoch() + static_cast<int64_t>(seconds * TimeStamp::kMicrosecondsPerSeconds_));
}

inline TimeStamp AddTimeUs(TimeStamp x, uint64_t us) noexcept {
  return TimeStamp(x.GetMicrosecondsSinceEpoch() + us);
}

inline TimeStamp AddTimeMs(TimeStamp x, uint64_t ms) noexcept {
  return AddTimeUs(x, ms * 1000);
}

} // namespace kanon

#endif // KANON_TIMESTAMP_H
