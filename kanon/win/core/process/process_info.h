#ifndef KANON_WIN_CORE_PROCESS_PROCESS_INFO_H__
#define KANON_WIN_CORE_PROCESS_PROCESS_INFO_H__

#include <windows.h>
#include <processthreadsapi.h>
#include "kanon/util/macro.h"

namespace kanon {
namespace process {

using PId = DWORD;

KANON_INLINE PId Pid() noexcept { return ::GetCurrentProcessId(); }

} // namespace process
} // namespace kanon

#endif
