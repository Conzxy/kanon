#ifndef KANON_NET_UTIL_H__
#define KANON_NET_UTIL_H__

#include "kanon/util/compiler_macro.h"

namespace kanon {

KANON_NET_API void KanonNetInitialize();
KANON_NET_API void KanonNetTeardown() KANON_NOEXCEPT;

} // namespace kanon

#endif
