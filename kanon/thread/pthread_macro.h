#ifndef KANON_THREAD_MACRO_H
#define KANON_THREAD_MACRO_H

#include "kanon/util/check.h"

#include <assert.h>

#ifndef PTHREAD_CHECK

#undef ASSERT
#define ASSERT assert

#endif // PTHREAD_CHECK

#define TCHECK(ret) \
  ({ \
    ASSERT(((ret) == 0)); \
   })


#endif