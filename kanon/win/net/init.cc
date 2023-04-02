#include <winsock2.h>

#include "kanon/log/logger.h"
#include "kanon/net/init.h"

void kanon::KanonNetInitialize()
{
  WSAData wsa_data;
  const auto err = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (err == 0) {
    LOG_TRACE_KANON << "Kanon net resources initialization: OK";
    return;
  }
  LOG_ERROR_KANON << "Kanon net resources initialization: Failed";

  switch (err) {
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

void kanon::KanonNetTeardown() KANON_NOEXCEPT
{
  const auto ret = ::WSACleanup();

  if (ret == SOCKET_ERROR) {
    const auto err = ::WSAGetLastError();
    switch (err) {
      case WSANOTINITIALISED:
        LOG_ERROR_KANON << "KanonNetTeardown() has called successfully before "
                     "calling this function";
        break;
      case WSAENETDOWN:
        LOG_SYSFATAL << "The network system of host is down";
        break;
      case WSAEINPROGRESS:
        return;
      default:
        LOG_ERROR_KANON << "Kanon net resources teardown: Failed";
        LOG_SYSFATAL
            << "Unexpected error occurred when releasing net resources";
    }
  } else {
    LOG_TRACE_KANON << "Kanon net resources teardown: OK";
  }
}