#include "kanon/net/sock_api.h"

#include "kanon/log/logger.h"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

using namespace kanon;

static LPFN_ACCEPTEX accept_ex_fn = NULL;
static LPFN_CONNECTEX connect_ex_fn = NULL;
static GUID guid_accept_ex = WSAID_ACCEPTEX;
static GUID guid_connect_ex = WSAID_CONNECTEX;

void sock::Listen(FdType fd) KANON_NOEXCEPT
{
  auto ret = ::listen(fd, SOMAXCONN);

  if (ret < 0) {
    LOG_SYSFATAL << "listen error";
  }

  // Load the AcceptEx function into memory using WSAIoctl.
  // The WSAIoctl function is an extension of the ioctlsocket()
  // function that can use overlapped I/O. The function's 3rd
  // through 6th parameters are input and output buffers where
  // we pass the pointer to our AcceptEx function. This is used
  // so that we can call the AcceptEx function directly, rather
  // than refer to the Mswsock.lib library.
  if (!accept_ex_fn) {
    DWORD dw_bytes = 0;
    auto iResult =
        WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid_accept_ex,
                 sizeof(guid_accept_ex), &accept_ex_fn, sizeof(accept_ex_fn),
                 &dw_bytes, NULL, NULL);
    if (!accept_ex_fn || iResult == SOCKET_ERROR) {
      // wprintf(L"WSAIoctl failed with error: %u\n", WSAGetLastError());
      sock::Close(fd);
      WSACleanup();
      LOG_SYSFATAL << "Can't get the AcceptEx() symbol";
    }
  }
}

FdType sock::CreateNonBlockAndCloExecSocket(bool ipv6) KANON_NOEXCEPT
{
  auto sockfd = ::WSASocket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP,
                            NULL, 0, WSA_FLAG_OVERLAPPED);
  if (sockfd == INVALID_SOCKET) {
    LOG_SYSFATAL << "create new socket fd error";
    return INVALID_SOCKET;
  } else {
    LOG_DEBUG_KANON << "New socket = " << sockfd;
  }
  return sockfd;
}

FdType sock::CreateOverlappedSocket(bool ipv6) KANON_NOEXCEPT
{
  auto sockfd = ::WSASocket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP,
                            NULL, 0, WSA_FLAG_OVERLAPPED);
  if (sockfd == INVALID_SOCKET) {
    LOG_SYSFATAL << "create new socket fd error";
    return INVALID_SOCKET;
  } else {
    LOG_DEBUG_KANON << "New socket = " << sockfd;
  }
  return sockfd;
}

FdType sock::Accept(FdType fd, sockaddr_in6 *addr) KANON_NOEXCEPT
{
  socklen_t socklen = sizeof(struct sockaddr_in6);

  // Create an accepting socket
  auto cli_sock =
      sock::CreateNonBlockAndCloExecSocket(addr->sin6_family == AF_INET6);
  if (cli_sock == INVALID_SOCKET) {
    // wprintf(L"Create accept socket failed with error: %u\n",
    // WSAGetLastError());
    sock::Close(fd);
    WSACleanup();
    return cli_sock;
  }

  OVERLAPPED overlapp_buf;
  // Empty our overlapped structure and accept connections.
  memset(&overlapp_buf, 0, sizeof(overlapp_buf));
  DWORD recv_bytes = 0;

  if (accept_ex_fn) {
    auto bRetVal =
        accept_ex_fn(fd, cli_sock, nullptr, 0, sizeof(sockaddr_in6) + 16,
                     sizeof(sockaddr_in6) + 16, &recv_bytes, &overlapp_buf);
    int flag = 1;
    if (setsockopt(cli_sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                   (char const *)&flag, sizeof(flag)))
    {
      LOG_SYSERROR_KANON << "Failed to setsockopt(SO_UPDATE_ACCPET_CONTEXT)";
    }

    if (bRetVal == FALSE) {
      // wprintf(L"AcceptEx failed with error: %u\n", WSAGetLastError());
      sock::Close(cli_sock);
      sock::Close(fd);
      WSACleanup();
    }
  } else {
    LOG_FATAL << "Must call listen before accept";
  }

  return cli_sock;
}

bool sock::WinConnect(FdType fd, sockaddr const *addr,
                      CompletionContext *ctx) KANON_NOEXCEPT
{
  KANON_ASSERT(ctx, "The Completion Context of connect can't be null!");
  if (!connect_ex_fn) {
    DWORD dw_bytes = 0;
    auto iResult =
        WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid_connect_ex,
                 sizeof(guid_connect_ex), &connect_ex_fn, sizeof(connect_ex_fn),
                 &dw_bytes, NULL, NULL);
    if (!connect_ex_fn || iResult == SOCKET_ERROR) {
      LOG_SYSFATAL << "Can't get the ConnectEx() symbol";
      return false;
    }
  }

  bool bRetVal = false;
  if (connect_ex_fn) {
    if (addr->sa_family == AF_INET) {
      sockaddr_in dummy_addr;
      memset(&dummy_addr, 0, sizeof(dummy_addr));
      dummy_addr.sin_family = AF_INET;
      dummy_addr.sin_addr.s_addr = INADDR_ANY;
      dummy_addr.sin_port = 0;
      sock::Bind(fd, (sockaddr *)&dummy_addr);
    } else if (addr->sa_family == AF_INET6) {
      sockaddr_in6 dummy_addr;
      memset(&dummy_addr, 0, sizeof(dummy_addr));
      dummy_addr.sin6_family = AF_INET6;
      dummy_addr.sin6_addr = in6addr_any;
      dummy_addr.sin6_port = 0;
      sock::Bind(fd, (sockaddr *)&dummy_addr);
    } else {
      LOG_SYSFATAL << "The address used for connect is not supported";
    }

    // The socket argument of ConnectEx()
    // must bound and disconnected.
    // ie. You should call bind() to make calling
    // ConnectEx() successfully.
    DWORD send_bytes = 0;
    bRetVal =
        connect_ex_fn(fd, addr,
                      addr->sa_family == AF_INET ? sizeof(struct sockaddr_in)
                                                 : sizeof(struct sockaddr_in6),
                      nullptr, 0, &send_bytes, (LPOVERLAPPED)ctx);
  }

  return bRetVal;
}

int sock::GetSocketError(FdType fd) KANON_NOEXCEPT
{
  int optval;
  auto len = static_cast<socklen_t>(sizeof optval);

  auto ret = ::getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&optval, &len);
  if (!ret) {
    return optval;
  }

  assert(ret == SOCKET_ERROR);
  return ::WSAGetLastError();
}
