#ifndef KANON_NET_MACRO_H
#define KANON_NET_MACRO_H

#include "kanon/util/platform_macro.h"

#ifdef KANON_ON_LINUX
#include <linux/version.h>

namespace kanon {

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 44)
#define ENABLE_EPOLL
#endif

} // namespace kanon

#endif

#endif
