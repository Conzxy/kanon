#include "kanon/util/platform_macro.h"
#ifdef KANON_ON_WIN
#include "kanon/win/net/type.h"
#elif defined(KANON_ON_UNIX)
#include "kanon/linux/net/type.h"
#endif