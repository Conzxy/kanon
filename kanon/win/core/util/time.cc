#include "kanon/win/core/util/time.h"
#include <windows.h>
#include <stdint.h>
#include <sysinfoapi.h>
#include <timezoneapi.h>

// \see
// https://stackoverflow.com/questions/10905892/equivalent-of-gettimeofday-for-windows
//
// Note: some broken versions only have 8 trailing zero's, the correct epoch
// has 9 trailing zero's This magic number is the number of 100 nanosecond
// intervals since January 1, 1601 (UTC) until 00:00:00 January 1, 1970
static constexpr uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

int kanon::GetTimeOfDay(struct timeval *tv, struct timezone *tz) KANON_NOEXCEPT
{
  SYSTEMTIME system_time;
  FILETIME file_time;
  uint64_t time;

  GetSystemTime(&system_time);
  SystemTimeToFileTime(&system_time, &file_time);
  time = ((uint64_t)file_time.dwLowDateTime);
  time += ((uint64_t)file_time.dwHighDateTime) << 32;

  tv->tv_sec = (unsigned long long)((time - EPOCH) / 10000000L);
  tv->tv_usec = (unsigned long long)(system_time.wMilliseconds * 1000);
  return 0;
}
