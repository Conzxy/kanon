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
  , less_than_comparable<TimeStamp> {
  struct TimeUnitBase {
    explicit TimeUnitBase(int c)
      : count(c)
    {
    }

    int count;
  };

 public:
  struct MilliSecond : TimeUnitBase {
    explicit MilliSecond(int c)
      : TimeUnitBase(c)
    {
    }
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
    {
    }
  };

 public:
  TimeStamp() = default;

  explicit TimeStamp(int64_t microseconds)
    : microseconds_(microseconds)
  {
  }

  ~TimeStamp() = default;

  /*--------------------------------------------------*/
  /* String Conversion                                */
  /*--------------------------------------------------*/

  KANON_CORE_API std::string ToString() const;
  KANON_CORE_API std::string
  ToFormattedString(bool isShowMicroseconds = true) const;

  /*--------------------------------------------------*/
  /* Setter                                           */
  /*--------------------------------------------------*/

  void SetMicroseconds(int64_t us) KANON_NOEXCEPT { microseconds_ = us; }

  /*--------------------------------------------------*/
  /* Getter                                           */
  /*--------------------------------------------------*/

  int64_t GetMicrosecondsSinceEpoch() const KANON_NOEXCEPT { return microseconds_; }

  int64_t microseconds() const KANON_NOEXCEPT { return GetMicroseconds(); }
  int64_t milliseconds() const KANON_NOEXCEPT { return GetMilliseconds(); }
  int64_t seconds() const KANON_NOEXCEPT { return GetSeconds(); }

  int64_t GetMicroseconds() const KANON_NOEXCEPT { return microseconds_; }

  int64_t GetSeconds() const KANON_NOEXCEPT
  {
    return static_cast<int64_t>(microseconds_ / kMicrosecondsPerSeconds_);
  }

  int64_t GetMilliseconds() const KANON_NOEXCEPT
  {
    return static_cast<int64_t>(microseconds_ / 1000);
  }

  /*--------------------------------------------------*/
  /* Operator overloading                             */
  /* +, -, -(unary)                                   */
  /*--------------------------------------------------*/

  TimeStamp operator-(TimeStamp rhs) const KANON_NOEXCEPT
  {
    return TimeStamp(microseconds_ - rhs.microseconds_);
  }

  TimeStamp operator+(TimeStamp rhs) const KANON_NOEXCEPT
  {
    return TimeStamp(microseconds_ + rhs.microseconds_);
  }

  TimeStamp operator+(double seconds) const KANON_NOEXCEPT
  {
    return TimeStamp(microseconds_ + seconds * kMicrosecondsPerSeconds_);
  }

  TimeStamp operator+(Microsecond us) const KANON_NOEXCEPT
  {
    return TimeStamp(microseconds_ + us.count);
  }

  TimeStamp operator+(MilliSecond ms) const KANON_NOEXCEPT
  {
    return *this + Microsecond(ms.count * 1000);
  }

  TimeStamp operator+(Second sec) const KANON_NOEXCEPT
  {
    return *this + Microsecond(sec.count * 1000000);
  }

  TimeStamp operator-(double seconds) const KANON_NOEXCEPT
  {
    return TimeStamp(microseconds_ - seconds * kMicrosecondsPerSeconds_);
  }

  TimeStamp operator-(Microsecond us) const KANON_NOEXCEPT
  {
    return *this + Microsecond(-us.count);
  }

  TimeStamp operator-(MilliSecond ms) const KANON_NOEXCEPT
  {
    return *this + MilliSecond(-ms.count);
  }

  TimeStamp operator-(Second sec) const KANON_NOEXCEPT
  {
    return *this + Second(-sec.count);
  }

  friend TimeStamp operator+(double seconds, TimeStamp ts) KANON_NOEXCEPT
  {
    return ts + seconds;
  }

  friend TimeStamp operator+(TimeStamp::Microsecond us, TimeStamp ts) KANON_NOEXCEPT
  {
    return ts + us;
  }

  friend TimeStamp operator+(TimeStamp::MilliSecond ms, TimeStamp ts) KANON_NOEXCEPT
  {
    return ts + ms;
  }

  friend TimeStamp operator+(TimeStamp::Second sec, TimeStamp ts) KANON_NOEXCEPT
  {
    return ts + sec;
  }

  friend TimeStamp operator-(double seconds, TimeStamp ts) KANON_NOEXCEPT
  {
    return -(ts - seconds);
  }

  friend TimeStamp operator-(Microsecond us, TimeStamp ts) KANON_NOEXCEPT
  {
    return -(ts - us);
  }

  friend TimeStamp operator-(MilliSecond ms, TimeStamp ts) KANON_NOEXCEPT
  {
    return -(ts - ms);
  }

  friend TimeStamp operator-(Second sec, TimeStamp ts) KANON_NOEXCEPT
  {
    return -(ts - sec);
  }

  TimeStamp operator-() KANON_NOEXCEPT { return TimeStamp(-microseconds_); }

  /*--------------------------------------------------*/
  /* Utility                                          */
  /*--------------------------------------------------*/

  /**
   * \param rep Time representation string
   * \param format \see strptime
   * \param ok success indicator(optional)
   */
  KANON_CORE_API static TimeStamp
  FromTimeStr(char const *format, std::string const &rep, bool *ok = nullptr);

  static TimeStamp FromUnixTime(time_t seconds, int64_t microseconds) KANON_NOEXCEPT
  {
    return TimeStamp(static_cast<int64_t>(seconds * kMicrosecondsPerSeconds_ +
                                          microseconds));
  }

  static TimeStamp FromUnixTime(time_t seconds) KANON_NOEXCEPT
  {
    return FromUnixTime(seconds, 0);
  }

  KANON_CORE_API static TimeStamp Now() KANON_NOEXCEPT;

  static constexpr int kMicrosecondsPerSeconds_ = 1000000;

 private:
  int64_t microseconds_;
};

KANON_INLINE bool operator==(TimeStamp x, TimeStamp y) KANON_NOEXCEPT
{
  return x.GetMicrosecondsSinceEpoch() == y.GetMicrosecondsSinceEpoch();
}

KANON_INLINE bool operator<(TimeStamp x, TimeStamp y) KANON_NOEXCEPT
{
  return x.GetMicrosecondsSinceEpoch() < y.GetMicrosecondsSinceEpoch();
}

KANON_INLINE double TimeDifference(TimeStamp x, TimeStamp y) KANON_NOEXCEPT
{
  int64_t diff = x.GetMicrosecondsSinceEpoch() - y.GetMicrosecondsSinceEpoch();
  return static_cast<double>(diff) / TimeStamp::kMicrosecondsPerSeconds_;
}

KANON_INLINE TimeStamp AddTime(TimeStamp x, double seconds) KANON_NOEXCEPT
{
  return TimeStamp(
      x.GetMicrosecondsSinceEpoch() +
      static_cast<int64_t>(seconds * TimeStamp::kMicrosecondsPerSeconds_));
}

KANON_INLINE TimeStamp AddTimeUs(TimeStamp x, uint64_t us) KANON_NOEXCEPT
{
  return TimeStamp(x.GetMicrosecondsSinceEpoch() + us);
}

KANON_INLINE TimeStamp AddTimeMs(TimeStamp x, uint64_t ms) KANON_NOEXCEPT
{
  return AddTimeUs(x, ms * 1000);
}

} // namespace kanon

#endif // KANON_TIMESTAMP_H
