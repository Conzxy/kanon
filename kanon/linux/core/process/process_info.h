#ifndef KANON_LINUX_CORE_PROCESS_PROCESS_INFO_H__
#define KANON_LINUX_CORE_PROCESS_PROCESS_INFO_H__

#include <unistd.h>
#include "kanon/util/macro.h"

namespace kanon {

namespace process {

using PId = pid_t;

inline PId Pid() noexcept { return ::getpid(); }

} // namespace process
} // namespace kanon
#endif