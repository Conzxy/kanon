#ifndef KANON_UTIL_MEM_H
#define KANON_UTIL_MEM_H

#include <string.h>
#include "kanon/util/macro.h"

KANON_INLINE void MemoryZero(void *buf, size_t n) KANON_NOEXCEPT
{
  ::memset(buf, 0, n);
}

template <size_t N>
KANON_INLINE void MemoryZero(char (&buf)[N]) KANON_NOEXCEPT
{
  MemoryZero(buf, N);
}

template <typename T>
KANON_INLINE void MemoryZero(T &obj) KANON_NOEXCEPT
{
  ::memset(&obj, 0, sizeof(T));
}

#endif // KANON_UTIL_MEM_H