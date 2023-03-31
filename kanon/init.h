#include "kanon/util/macro.h"
#include "kanon/net/init.h"
#include "kanon/util/init.h"

namespace kanon {

KANON_INLINE void KanonInitialize()
{
  KanonCoreInitialize();
  KanonNetInitialize();
}

KANON_INLINE void KanonTeardown() KANON_NOEXCEPT
{
  KanonNetTeardown();
  KanonCoreTeardown();
}

struct KanonInitGuard {
  KanonInitGuard() { KanonInitialize(); }
  ~KanonInitGuard() KANON_NOEXCEPT { KanonTeardown(); }
};

#define KANON_INIT_GUARD                                                       \
  KanonInitGuard salkj2432974dfkhasuigalfg                                     \
  {                                                                            \
  }

} // namespace kanon
