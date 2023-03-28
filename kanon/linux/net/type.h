#ifndef KANON_LINUX_NET_TYPE_H__
#define KANON_LINUX_NET_TYPE_H__

#include <sys/socket.h>

namespace kanon {

using FdType = int;

enum ShutdownDirection {
  SHUT_READ = SHUT_RD,
  SHUT_WRITE = SHUT_WR,
  SHUT_BOTH = SHUT_RDWR,
};

} // namespace kanon

#endif