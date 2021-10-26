#include "current_thread.h"
#include "kanon/string/lexical-cast.h"

#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

// In C++17
// you can write as:
// namespace kanon::CurrentThread
//
namespace kanon {

namespace CurrentThread {

__thread int t_tid = 0;
__thread char t_tidString[32] = {};
__thread int t_tidLength = 0;
__thread char const* t_name = nullptr; 

// because main thread is not created by pthread_create()
// need explicit cache
struct MainThreadInit {
	MainThreadInit()
	{
		CurrentThread::t_name = "main";
		CurrentThread::tid();

	}
};

MainThreadInit mainThreadInit{};

pid_t gettid() 
{ return ::syscall(SYS_gettid); }

void cacheTid() KANON_NOEXCEPT
{
	if( __builtin_expect(t_tid == 0, 1) ) {
		t_tid = gettid();
		strcpy(t_tidString, lexical_cast<char const*>(t_tid));
	}
}

bool isMainThread() KANON_NOEXCEPT
{ return CurrentThread::t_tid == ::getpid(); }

} // namespace CurrentThread

} // namespace kanon
