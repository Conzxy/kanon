#include "time_stamp.h"

#include <string.h>
#include <time.h>
#include <inttypes.h>

#include "kanon/util/time.h"

namespace kanon {

TimeStamp TimeStamp::Now() noexcept
{
  struct timeval time;
  GetTimeOfDay(&time, NULL);
  return TimeStamp(static_cast<int64_t>(time.tv_sec * kMicrosecondsPerSeconds_ +
                                        time.tv_usec));
}

std::string TimeStamp::ToString() const
{
  char buf[64]{0};

  int64_t seconds = microseconds_ / kMicrosecondsPerSeconds_;
  int microseconds = static_cast<int>(microseconds_ % kMicrosecondsPerSeconds_);
  snprintf(buf, sizeof buf, "%" PRId64 ".6%d", seconds, microseconds);

  return buf;
}

std::string TimeStamp::ToFormattedString(bool isShowMicroseconds) const
{
  char buf[64]{0};

  auto seconds = static_cast<time_t>(microseconds_ / kMicrosecondsPerSeconds_);
  struct tm tm;
  // gmtime_r(&seconds, &tm);
#ifdef KANON_ON_UNIX
  ::localtime_r(&seconds, &tm);
#else
  ::localtime_s(&tm, &seconds);
#endif
  if (isShowMicroseconds) {
    int microseconds =
        static_cast<int>(microseconds_ % kMicrosecondsPerSeconds_);

    snprintf(buf, sizeof buf, "%4d%02d%02d %02d:%02d:%02d.%06d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
             tm.tm_min, tm.tm_sec, microseconds);
  } else {
    snprintf(buf, sizeof buf, "%04d%02d%02d %02d:%02d:%02d", tm.tm_year + 1900,
             tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  }

  return buf;
}

TimeStamp TimeStamp::FromTimeStr(char const *format, std::string const &rep,
                                 bool *ok)
{
#ifdef KANON_ON_UNIX
  struct tm tm;
  memset(&tm, 0, sizeof tm);
  // Must init
  if (!strptime(rep.c_str(), format, &tm)) {
    if (ok) {
      *ok = false;
      return TimeStamp(0);
    }
  }
  time_t t = mktime(&tm);
  if (ok) *ok = true;
#else
  auto t = 0;
#endif
  return TimeStamp(t * TimeStamp::kMicrosecondsPerSeconds_);
}

} // namespace kanon
