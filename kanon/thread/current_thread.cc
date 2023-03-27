#include "kanon/thread/current_thread.h"

#include <cstring>

#include "kanon/process/process_info.h"
#include "kanon/string/lexical_cast.h"

#ifdef KANON_ON_WIN
#include "kanon/win/core/thread/current_thread.inl.h"
#elif defined(KANON_ON_UNIX)
#include "kanon/linux/core/thread/current_thread.inl.h"
#endif

// In C++17
// you can write as:
// namespace kanon::CurrentThread
//
namespace kanon {

namespace CurrentThread {

KANON_TLS int t_tid = 0;
KANON_TLS char t_tidString[32] = {};
KANON_TLS int t_tidLength = 0;
KANON_TLS char const *t_name = nullptr;

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

void cacheTid() noexcept
{
  t_tid = gettid();
  auto view = lexical_cast<StringView>(t_tid);
  t_tidLength = view.size();
  strncpy(t_tidString, view.data(), view.size());
}

KANON_INLINE bool isMainThread() noexcept
{
  return CurrentThread::t_tid == process::Pid();
}

} // namespace CurrentThread
} // namespace kanon
