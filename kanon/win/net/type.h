#ifndef KANON_WIN_NET_TYPE_H__
#define KANON_WIN_NET_TYPE_H__

#include <winsock2.h>

namespace kanon {

using FdType = SOCKET;

enum ShutdownDirection {
  SHUT_READ = SD_RECEIVE,
  SHUT_WRITE = SD_SEND,
  SHUT_BOTH = SD_BOTH,
};

} // namespace kanon

#endif
