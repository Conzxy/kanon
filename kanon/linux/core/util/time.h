#ifndef KANON_LINUX_CORE_UTIL_TIME_H__
#define KANON_LINUX_CORE_UTIL_TIME_H__

#include <sys/time.h>
#include "kanon/util/macro.h"

namespace kanon {

using ::timeval;
using ::timezone;

/**
 * It seems linux define a global variable "timezone"
 * in some headers, so timezone must use struct tag.
 */
KANON_INLINE int GetTimeOfDay(timeval *tv, struct timezone *ts) KANON_NOEXCEPT
{
  return ::gettimeofday(tv, ts);
}

} // namespace kanon

#endif
