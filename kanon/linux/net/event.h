#ifndef KANON_LINUX_NET_EVENT_H__
#define KANON_LINUX_NET_EVENT_H__

// Obtain POLLRDHUP
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <poll.h>

#ifdef ENABLE_EPOLL
#include <sys/epoll.h>
static_assert(EPOLLIN == POLLIN, "EPOLLIN should equal to POLLIN");
static_assert(EPOLLOUT == POLLOUT, "EPOLLOUT should equal to POLLOUT");
static_assert(EPOLLPRI == POLLPRI, "EPOLLPRI should equal to POLLRI");
#endif

namespace kanon {

enum Event {
  ReadEvent = POLLIN | POLLPRI,
  WriteEvent = POLLOUT,
  NoneEvent = 0
};

} // namespace kanon

#endif