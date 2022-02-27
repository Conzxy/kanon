#ifndef KANON_CHECK_H
#define KANON_CHECK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace kanon {

#ifndef likely 
#define likely __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif 

#define ASSERT(ret) \
  ({if (unlikely(ret == 0)) \
   {\
    char buf[1536]; \
    ::memset(buf, 0, sizeof buf); \
    ::snprintf(buf, sizeof buf, "errno: %d; error message: %s; %s; %d; %s\n", \
        ret, strerror(ret), __FILE__, __LINE__, __func__); \
    ::fputs(buf, stderr); \
    ::fflush(stderr); \
    abort();\
   }})

} // namespace kanon

#endif // KANON_CHECK_H
