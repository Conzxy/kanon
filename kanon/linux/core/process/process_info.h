#ifndef KANON_LINUX_CORE_PROCESS_PROCESS_INFO_H__
#define KANON_LINUX_CORE_PROCESS_PROCESS_INFO_H__

#include <unistd.h>
#include "kanon/util/macro.h"

namespace kanon {
namespace process {

using PId = pid_t;

KANON_INLINE PId Pid() KANON_NOEXCEPT { return ::getpid(); }

} // namespace process
} // namespace kanon
#endif
