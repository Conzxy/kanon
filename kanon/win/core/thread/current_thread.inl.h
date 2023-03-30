#include <windows.h>
#include <processthreadsapi.h>
#include "kanon/util/macro.h"
#include "kanon/process/process_info.h"

namespace kanon {
namespace CurrentThread {

KANON_INLINE process::PId gettid() KANON_NOEXCEPT { return ::GetCurrentThreadId(); }

} // namespace CurrentThread
} // namespace kanon