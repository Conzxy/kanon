#ifndef KANON_THREAD_MACRO_H
#define KANON_THREAD_MACRO_H

#include "kanon/util/check.h"

#define TCHECK(ret) \
  ({ \
    ASSERT(ret); \
   })

#endif