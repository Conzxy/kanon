#ifndef KANON_NET_TYPE_H
#define KANON_NET_TYPE_H

namespace kanon {

template<typename T>
class EventLoopT;

class Poller;
class Epoller;

#ifdef KANON_ENABLE_EPOLL
typedef EventLoopT<Epoller> EventLoop;
#else
typedef EventLoopT<Poller> EventLoop;
#endif
}

#endif 
