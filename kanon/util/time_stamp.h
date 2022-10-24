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
  struct TimeUnitBase {
    explicit TimeUnitBase(int c)
      : count(c)
    {}

    int count;
  };
  
 public:
  struct MilliSecond : TimeUnitBase {
    explicit MilliSecond(int c)
      : TimeUnitBase(c)
    {}
  };

  struct Microsecond : TimeUnitBase {
    explicit Microsecond(int c)
      : TimeUnitBase(c)
    {
    }
  };

  struct Second : TimeUnitBase {
    explicit Second(int c)
      : TimeUnitBase(c)
    {}
  };

public:
  TimeStamp() = default;

  explicit TimeStamp(int64_t microseconds)
    : microseconds_(microseconds)
  { }

  ~TimeStamp() = default;

  /*--------------------------------------------------*/
  /* String Conversion                                */
  /*--------------------------------------------------*/

  std::string ToString() const;
  std::string ToFormattedString(bool isShowMicroseconds = true) const;
  
  /*--------------------------------------------------*/
  /* Setter                                           */
  /*--------------------------------------------------*/

  void SetMicroseconds(int64_t us) noexcept 
  {
    microseconds_ = us;
  }
 
  /*--------------------------------------------------*/
  /* Getter                                           */
  /*--------------------------------------------------*/

  int64_t GetMicrosecondsSinceEpoch() const noexcept
  { return microseconds_; }
  
  int64_t microseconds() const noexcept { return GetMicroseconds(); }
  int64_t milliseconds() const noexcept { return GetMilliseconds(); }
  int64_t seconds() const noexcept { return GetSeconds(); }
  
  int64_t GetMicroseconds() const noexcept
  {
    return microseconds_;
  }

  int64_t GetSeconds() const noexcept 
  {
    return static_cast<int64_t>(microseconds_ / kMicrosecondsPerSeconds_);
  }

  int64_t GetMilliseconds() const noexcept
  {
    return static_cast<int64_t>(microseconds_ / 1000);
  }
  
  /*--------------------------------------------------*/
  /* Operator overloading                             */
  /* +, -, -(unary)                                   */
  /*--------------------------------------------------*/
  
  TimeStamp operator-(TimeStamp rhs) const noexcept
  {
    return TimeStamp(microseconds_ - rhs.microseconds_);
  }

  TimeStamp operator+(TimeStamp rhs) const noexcept
  {
    return TimeStamp(microseconds_ + rhs.microseconds_);
  }

  TimeStamp operator+(double seconds) const noexcept
  {
    return TimeStamp(microseconds_ + seconds * kMicrosecondsPerSeconds_);
  }
  
  TimeStamp operator+(Microsecond us) const noexcept
  {
    return TimeStamp(microseconds_ + us.count);
  }

  TimeStamp operator+(MilliSecond ms) const noexcept
  {
    return *this + Microsecond(ms.count * 1000);
  }

  TimeStamp operator+(Second sec) const noexcept
  {
    return *this + Microsecond(sec.count * 1000000);
  }

  TimeStamp operator-(double seconds) const noexcept
  {
    return TimeStamp(microseconds_ - seconds * kMicrosecondsPerSeconds_);
  }

  TimeStamp operator-(Microsecond us) const noexcept
  {
    return *this + Microsecond(-us.count);
  }

  TimeStamp operator-(MilliSecond ms) const noexcept
  {
    return *this + MilliSecond(-ms.count);
  }

  TimeStamp operator-(Second sec) const noexcept
  {
    return *this + Second(-sec.count);
  }

  friend TimeStamp operator+(double seconds, TimeStamp ts) noexcept
  {
    return ts + seconds;
  }

  friend TimeStamp operator+(TimeStamp::Microsecond us, TimeStamp ts) noexcept
  {
    return ts + us;
  }

  friend TimeStamp operator+(TimeStamp::MilliSecond ms, TimeStamp ts) noexcept
  {
    return ts + ms;
  }

  friend TimeStamp operator+(TimeStamp::Second sec, TimeStamp ts) noexcept
  {
    return ts + sec;
  }

  friend TimeStamp operator-(double seconds, TimeStamp ts) noexcept
  {
    return -(ts - seconds);
  }

  friend TimeStamp operator-(Microsecond us, TimeStamp ts) noexcept
  {
    return -(ts - us);
  }

  friend TimeStamp operator-(MilliSecond ms, TimeStamp ts) noexcept
  {
    return -(ts - ms);
  }

  friend TimeStamp operator-(Second sec, TimeStamp ts) noexcept
  {
    return -(ts - sec);
  }
  
  TimeStamp operator-() noexcept
  {
    return TimeStamp(-microseconds_);
  }
  
  /*--------------------------------------------------*/
  /* Utility                                          */
  /*--------------------------------------------------*/
 
  /**
   * \param rep Time representation string
   * \param format \see strptime
   * \param ok success indicator(optional)
   */
  static TimeStamp FromTimeStr(char const *format, std::string const &rep, bool *ok=nullptr);

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
