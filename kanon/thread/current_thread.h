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
extern __thread int t_tid;
extern __thread char t_tidString[32]; 
extern __thread int t_tidLength;
extern __thread char const* t_name;

void cacheTid() KANON_NOEXCEPT;

inline int tid() KANON_NOEXCEPT
{
  if( __builtin_expect(t_tid == 0, 0) )
    cacheTid();
  return t_tid;
}

inline char const* tidString() KANON_NOEXCEPT
{ return t_tidString; }

inline int tidLength() KANON_NOEXCEPT
{ return t_tidLength; }

inline char const* name() KANON_NOEXCEPT
{ return t_name; }

bool isMainThread() KANON_NOEXCEPT;

} // namespace CurrentThread
} // namespace kanon

#endif // KANON_CURRENT_THREAD_H
