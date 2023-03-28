#include "kanon/util/macro.h"
#include "kanon/net/init.h"
#include "kanon/util/init.h"

namespace kanon {

KANON_INLINE void KanonInitialize()
{
  KanonCoreInitialize();
  KanonNetInitialize();
}

} // namespace kanon
