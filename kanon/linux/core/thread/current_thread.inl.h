#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "kanon/linux/core/process/process_info.h"

namespace kanon {
namespace CurrentThread {

/**
 * I don't use gettid() since I don't like it, because:
 * 1. gettid() is provided when glibc >= 2.30
 * 2. gettid() manpage just include <sys/type.h>
 *    but in  fact, you should include <unistd.h> also.
 *
 * \see
 * https://stackoverflow.com/questions/30680550/c-gettid-was-not-declared-in-this-scope
 * man gettid
 */
KANON_INLINE process::PId gettid()
{
  return (process::PId)::syscall(SYS_gettid);
}

} // namespace CurrentThread
} // namespace kanon
