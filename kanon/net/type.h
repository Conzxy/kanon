#ifndef KANON_NET_TYPE_H
#define KANON_NET_TYPE_H

#include <linux/version.h>

namespace kanon {

template<typename T>
class EventLoopT;

class Poller;
class Epoller;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 44) 
typedef EventLoopT<Poller> EventLoop;
#else
typedef EventLoopT<Epoller> EventLoop;
#endif

}

#endif 
