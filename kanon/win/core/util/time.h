#ifndef KANON_WIN_UTIL_TIME_H__
#define KANON_WIN_UTIL_TIME_H__

#include <ctime>

#include "kanon/util/macro.h"

namespace kanon {

/** Although Win SDK has a timeval definition in <winsock.h>
    But I don't like it */
struct timeval {
  /** !WARNING
    Don't use unsigned long
    In MSVC 64bits, it it 4 bytes */
  unsigned long long tv_sec;
  unsigned long long tv_usec;
};

/** Win SDK no timezone definition */
struct timezone {
  int tz_minuteswest; /* minutes W of Greenwich */
  int tz_dsttime;     /* type of dst correction */
};

/** Equivalent implementation of gettimeofday() */
KANON_CORE_API int GetTimeOfDay(struct timeval *tv,
                                struct timezone *ts) KANON_NOEXCEPT;

} // namespace kanon

#endif