#ifndef KANON_CURRENT_THREAD_H
#define KANON_CURRENT_THREAD_H

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

void cacheTid() noexcept;

inline int tid() noexcept
{
	if( __builtin_expect(t_tid == 0, 0) )
		cacheTid();
	return t_tid;
}

inline char const* tidString() noexcept
{ return t_tidString; }

inline int tidLength() noexcept
{ return t_tidLength; }

inline char const* name() noexcept
{ return t_name; }

bool isMainThread() noexcept;

} // namespace CurrentThread
} // namespace kanon

#endif // KANON_CURRENT_THREAD_H
