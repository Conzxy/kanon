#ifndef KANON_UTIL_INIT_H__
#define KANON_UTIL_INIT_H__

#include "kanon/util/macro.h"

namespace kanon {

KANON_CORE_API void KanonCoreInitialize();
KANON_CORE_API void KanonCoreTeardown() KANON_NOEXCEPT;

} // namespace kanon

#endif