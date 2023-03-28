#ifndef KANON_UTIL_ENDIAN_API_H__
#define KANON_UTIL_ENDIAN_API_H__

#include "kanon/util/macro.h"

#ifdef KANON_ON_WIN
#include "kanon/win/core/util/endian_api.h"
#elif defined(KANON_ON_UNIX)
#include "kanon/linux/core/util/endian_api.h"
#endif

#endif