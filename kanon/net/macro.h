#ifndef KANON_NET_TYPE_H
#define KANON_NET_TYPE_H

#include <linux/version.h>

namespace kanon {

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 44) 
#define ENABLE_EPOLL
#endif

}

#endif 
