#ifndef KANON_THREAD_MACRO_H
#define KANON_THREAD_MACRO_H

#include "kanon/util/check.h"

#include <assert.h>

#define TCHECK(ret) \
  ({ \
    ASSERT(ret); \
   })


#endif