#ifndef KANON_UTIL_MEM_H
#define KANON_UTIL_MEM_H

#include <string.h>

inline void MemoryZero(void* buf, size_t n) noexcept
{
  ::memset(buf, 0, n);
}

template<size_t N>
inline void MemoryZero(char (&buf)[N]) noexcept
{
  MemoryZero(buf, N);
}

template<typename T>
inline void MemoryZero(T& obj) noexcept
{
  ::memset(&obj, 0, sizeof(T));
}

#endif // KANON_UTIL_MEM_H