#ifndef KANON_CURRENT_THREAD_H
#define KANON_CURRENT_THREAD_H

#include "kanon/util/macro.h"

namespace kanon {

// __thread requires file scope(i.e. global scope)
// so use namespace instead of class
namespace CurrentThread {
// __thread can declare thread-independent variable(only POD-type)
//
// for logging

// FIXME pid_t?
extern KANON_TLS int t_tid;
extern KANON_TLS char t_tidString[32];
extern KANON_TLS int t_tidLength;
extern KANON_TLS char const *t_name;

void cacheTid() noexcept;

inline int tid() noexcept
{
  if (KANON_UNLIKELY(t_tid == 0)) {
    cacheTid();
  }
  return t_tid;
}

inline char const *tidString() noexcept { return t_tidString; }

inline int tidLength() noexcept { return t_tidLength; }

inline char const *GetName() noexcept { return t_name; }

bool isMainThread() noexcept;

void MainThreadInitialize();

} // namespace CurrentThread
} // namespace kanon

#endif // KANON_CURRENT_THREAD_H
