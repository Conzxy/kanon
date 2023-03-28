#include "kanon/util/macro.h"

#ifdef KANON_ON_WIN
#include "kanon/win/core/util/time.h"
#elif defined(KANON_ON_UNIX)
#include "kanon/linux/core/util/time.h"
#endif