#include <winsock2.h>

#include "kanon/log/logger.h"
#include "kanon/net/init.h"

void kanon::KanonNetInitialize()
{
  WSAData wsa_data;
  auto err = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  switch (err) {
    case 0:
      LOG_INFO << "Kanon Net Module initialize successfully";
      return;

    case WSASYSNOTREADY:
      LOG_SYSFATAL << "The network system isn't ready";
      break;
    case WSAVERNOTSUPPORTED:
      LOG_SYSFATAL << "Not a support version of windows socket";
      break;
    case WSAEPROCLIM:
      LOG_SYSFATAL << "The limit of the number of tasks has reached";
      break;
    case WSAEFAULT:
      LOG_SYSFATAL << "wsa data is invalid";
      break;
  }
}