#ifndef KANON_WIN_UTIL_TIME_H__
#define KANON_WIN_UTIL_TIME_H__

#include <ctime>

namespace kanon {

struct timeval {
  /** !WARNING
    Don't use unsigned long
    In MSVC 64bits, it it 4 bytes */
  unsigned long long tv_sec;
  unsigned long long tv_usec;
};

struct timezone {
  int tz_minuteswest; /* minutes W of Greenwich */
  int tz_dsttime;     /* type of dst correction */
};

int GetTimeOfDay(struct timeval *tv, struct timezone *ts) noexcept;

} // namespace kanon

#endif