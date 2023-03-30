#ifndef KANON_STRINT_STRCASECMP_H__
#define KANON_STRING_STRCASECMP_H__

#include "kanon/util/compiler_macro.h"

namespace kanon {

KANON_CORE_API int StrCaseCompare(char const *s1, char const *s2);
KANON_CORE_API int StrNCaseCompare(char const *s1, char const *s2, size_t n);

} // namespace kanon

#endif
