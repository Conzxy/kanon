#ifndef KANON_PROCESS_PROCESS_INFO_H
#define KANON_PROCESS_PROCESS_INFO_H

#include <unistd.h>

#include "kanon/util/macro.h"
#include "kanon/string/string_view.h"

namespace kanon {

namespace process {
 
inline pid_t Pid() noexcept {
  return ::getpid();
}

StringView PidString() noexcept;

StringView Hostname() noexcept;

} // namespace process
} // namespace kanon

#endif // KANON_PROCESS_PROCESS_INFO_H
