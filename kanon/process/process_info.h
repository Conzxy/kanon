#ifndef KANON_PROCESS_PROCESS_INFO_H
#define KANON_PROCESS_PROCESS_INFO_H

#include "kanon/util/macro.h"

#ifdef KANON_ON_UNIX
#include "kanon/linux/core/process/process_info.h"
#elif defined(KANON_ON_WIN)
#include "kanon/win/core/process/process_info.h"
#endif

#include "kanon/string/string_view.h"

namespace kanon {
namespace process {

KANON_CORE_API StringView PidString() KANON_NOEXCEPT;
KANON_CORE_API StringView Hostname() KANON_NOEXCEPT;

} // namespace process
} // namespace kanon

#endif // KANON_PROCESS_PROCESS_INFO_H
