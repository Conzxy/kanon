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

#if !KANON___THREAD_DEFINED
void cacheTid() KANON_NOEXCEPT
{
  t_tid = gettid();
  auto view = lexical_cast<StringView>(t_tid);
  t_tidLength = view.size();
  strncpy(t_tidString, view.data(), view.size());
}

int tid() KANON_NOEXCEPT
{
  if (KANON_UNLIKELY(t_tid == 0)) {
    cacheTid();
  }
  return t_tid;
}

int GetTid() KANON_NOEXCEPT { return t_tid; }

char const *tidString() KANON_NOEXCEPT { return t_tidString; }
int tidLength() KANON_NOEXCEPT { return t_tidLength; }
char const *tidName() KANON_NOEXCEPT { return t_name; }

bool isMainThread() KANON_NOEXCEPT
{
  return CurrentThread::t_tid == process::Pid();
}
#endif //! !KANON___THREAD_DEFINED

// because main thread is not created by pthread_create()
// need explicit cache
struct MainThreadInit {
  MainThreadInit()
  {
    t_name = "main";
    cacheTid();
  }
};

void MainThreadInitialize()
{
  t_name = "main";
  cacheTid();
}

/* To compatible old code, don't remove this.
   In new code, you should call KanonCoreInitialize()
   to init global states */
MainThreadInit mainThreadInit{};

} // namespace CurrentThread
} // namespace kanon
