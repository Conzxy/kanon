#ifndef KANON_CURRENT_THREAD_H
#define KANON_CURRENT_THREAD_H

#include "kanon/util/macro.h"
#include "kanon/process/process_info.h"
#include "kanon/string/lexical_cast.h"

#ifdef KANON_ON_WIN
#  include "kanon/win/core/thread/current_thread.inl.h"
#elif defined(KANON_ON_UNIX)
#  include "kanon/linux/core/thread/current_thread.inl.h"
#endif

namespace kanon {

// __thread requires file scope(i.e. global scope)
// so use namespace instead of class
namespace CurrentThread {
// __thread can declare thread-independent variable(only POD-type)
//
// for logging

/**
 * In MSVC,
 * __declspec(thread) can't used with __declspec(dllexport)
 * \see
 * https://learn.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/compiler-error-c2492?view=msvc-170
 */
// FIXME pid_t?
extern KANON_TLS int t_tid;
extern KANON_TLS char t_tidString[32];
extern KANON_TLS int t_tidLength;
extern KANON_TLS char const *t_name;

#if KANON___THREAD_DEFINED
KANON_INLINE void cacheTid() KANON_NOEXCEPT
{
  t_tid = gettid();
  char buf[64];
  t_tidLength = snprintf(buf, sizeof buf, "%d", t_tid);
  strncpy(t_tidString, buf, t_tidLength);
}

KANON_INLINE int tid() KANON_NOEXCEPT
{
  if (KANON_UNLIKELY(t_tid == 0)) {
    cacheTid();
  }
  return t_tid;
}

KANON_INLINE char const *tidString() KANON_NOEXCEPT { return t_tidString; }
KANON_INLINE int tidLength() KANON_NOEXCEPT { return t_tidLength; }
KANON_INLINE int GetTid() KANON_NOEXCEPT { return t_tid; }
KANON_INLINE char const *tidName() KANON_NOEXCEPT { return t_name; }
KANON_INLINE bool isMainThread() KANON_NOEXCEPT
{
  return CurrentThread::t_tid == process::Pid();
}
#else

KANON_CORE_API void cacheTid() KANON_NOEXCEPT;
KANON_CORE_API int tid() KANON_NOEXCEPT;
KANON_CORE_API char const *tidString() KANON_NOEXCEPT;
KANON_CORE_API int tidLength() KANON_NOEXCEPT;
KANON_CORE_API int GetTid() KANON_NOEXCEPT;
KANON_CORE_API char const *tidName() KANON_NOEXCEPT;
KANON_CORE_API bool isMainThread() KANON_NOEXCEPT;
KANON_CORE_NO_API void MainThreadInitialize();
#endif

void MainThreadInitialize();

} // namespace CurrentThread
} // namespace kanon

#endif // KANON_CURRENT_THREAD_H
