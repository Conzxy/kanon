#ifndef KANON_LINUX_CORE_UTIL_TIME_H__
#define KANON_LINUX_CORE_UTIL_TIME_H__

#include "kanon/util/macro.h"
#include <sys/time.h>

namespace kanon {

KANON_INLINE int GetTimeOfDay(struct timeval *tv, struct timezone *ts) KANON_NOEXCEPT
{
  return ::gettimeofday(tv, ts);
}

} // namespace kanon
#endif