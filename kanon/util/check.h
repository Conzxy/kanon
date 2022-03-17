#ifndef KANON_CHECK_H
#define KANON_CHECK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

namespace kanon {

#ifndef likely 
#define likely __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif 

// Not need #ret
// We just see the error of specific errno
#define ASSERT(ret) \
  ({if (unlikely((ret) != 0)) \
   {\
    char buf[1536]; \
    ::memset(buf, 0, sizeof buf); \
    ::snprintf(buf, sizeof buf, "Aborted.\n%s() (Errno: %d, %s) - %s:%d\n", \
        __func__, ret, ::strerror(ret), __FILE__, __LINE__) ; \
    ::fputs(buf, stderr); \
    ::fflush(stderr); \
    ::abort();\
   }})

} // namespace kanon

#endif // KANON_CHECK_H
