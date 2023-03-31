#include "kanon/util/macro.h"
#include "kanon/net/init.h"
#include "kanon/util/init.h"

namespace kanon {

/**
 * \ingroup kanon
 * \addtogroup init
 * \brief Initialize and teardown functions
 * @{
 */

/**
 * \brief Initialize all global state of kanon
 *
 * \note
 *   Don't return values to indicates whether initialization is success or not.
 *   Also, don't throw exception.
 *   If any error occurred, just log some message and abort.
 */
KANON_INLINE void KanonInitialize() KANON_NOEXCEPT
{
  KanonCoreInitialize();
  KanonNetInitialize();
}

/**
 * \brief Initialize all global state of kanon
 *
 * \note
 *   Don't return values to indicates whether teardown is success or not.
 *   Also, don't throw exception.
 *   If any error occurred, just log some message and abort.
 */
KANON_INLINE void KanonTeardown() KANON_NOEXCEPT
{
  KanonNetTeardown();
  KanonCoreTeardown();
}

/**
 * A convenient(RAII) class to release all resource in case
 * user forget to call KanonTeardown()
 */
struct KanonInitGuard {
  KanonInitGuard() { KanonInitialize(); }
  ~KanonInitGuard() KANON_NOEXCEPT { KanonTeardown(); }
};

/**
 * A convenient macro to define a dummy variable 
 * to call KanonTeardown()
 */
#define KANON_INIT_GUARD                                                       \
  KanonInitGuard salkj2432974dfkhasuigalfg                                     \
  {                                                                            \
  }

//!@}

} // namespace kanon
