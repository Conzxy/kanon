#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

namespace kanon {
namespace CurrentThread {

static KANON_INLINE process::PId gettid() { return ::syscall(SYS_gettid); }

} // namespace CurrentThread
} // namespace kanon
