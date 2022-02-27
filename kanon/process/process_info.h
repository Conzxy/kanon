#ifndef KANON_PROCESS_PROCESS_INFO_H
#define KANON_PROCESS_PROCESS_INFO_H

#include "kanon/util/macro.h"
#include "kanon/string/string_view.h"

#include <unistd.h>

namespace kanon {

namespace process {
 
inline pid_t pid() noexcept {
  return ::getpid();
}

StringView pidString() noexcept;

StringView hostname() noexcept;

} // namespace process
} // namespace kanon

#endif // KANON_PROCESS_PROCESS_INFO_H
