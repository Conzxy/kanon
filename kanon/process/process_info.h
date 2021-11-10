#ifndef KANON_PROCESS_PROCESS_INFO_H
#define KANON_PROCESS_PROCESS_INFO_H

#include "kanon/util/macro.h"
#include "kanon/string/string_view.h"

#include <unistd.h>

namespace kanon {

namespace process {
 
inline pid_t pid() KANON_NOEXCEPT {
  return ::getpid();
}

StringView pidString() KANON_NOEXCEPT;

StringView hostname() KANON_NOEXCEPT;

} // namespace process
} // namespace kanon

#endif // KANON_PROCESS_PROCESS_INFO_H
