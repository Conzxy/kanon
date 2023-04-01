#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "kanon/linux/core/process/process_info.h"

namespace kanon {
namespace CurrentThread {

KANON_INLINE process::PId gettid() { return ::syscall(SYS_gettid); }

} // namespace CurrentThread
} // namespace kanon
