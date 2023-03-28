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

void sock::Listen(FdType fd) noexcept
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

FdType sock::CreateNonBlockAndCloExecSocket(bool ipv6) noexcept
{
  auto sockfd = ::WSASocket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP,
                            NULL, 0, WSA_FLAG_OVERLAPPED);
  if (sockfd == INVALID_SOCKET) {
    LOG_SYSFATAL << "create new socket fd error";
    return INVALID_SOCKET;
  } else {
    LOG_DEBUG << "New socket = " << sockfd;
  }
  return sockfd;
}

FdType sock::CreateOverlappedSocket(bool ipv6) noexcept
{
  auto sockfd = ::WSASocket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP,
                            NULL, 0, WSA_FLAG_OVERLAPPED);
  if (sockfd == INVALID_SOCKET) {
    LOG_SYSFATAL << "create new socket fd error";
    return INVALID_SOCKET;
  } else {
    LOG_DEBUG << "New socket = " << sockfd;
  }
  return sockfd;
}

FdType sock::Accept(FdType fd, sockaddr_in6 *addr) noexcept
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
      LOG_SYSERROR << "Failed to setsockopt(SO_UPDATE_ACCPET_CONTEXT)";
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
                      CompletionContext *ctx) noexcept
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
  OVERLAPPED overlapp_buf;
  memset(&overlapp_buf, 0, sizeof(overlapp_buf));
  // DWORD recv_bytes = 0;

  bool bRetVal = false;
  if (connect_ex_fn) {
    sockaddr_in dummy_addr;
    dummy_addr.sin_family = AF_INET;
    dummy_addr.sin_addr.s_addr = INADDR_ANY;
    dummy_addr.sin_port = 0;
    sock::Bind(fd, (sockaddr *)&dummy_addr);

    // The socket argument of ConnectEx()
    // must bound and disconnected.
    // ie. You should call bind() to make calling
    // ConnectEx() successfully.
    bRetVal =
        connect_ex_fn(fd, addr,
                      addr->sa_family == AF_INET ? sizeof(struct sockaddr_in)
                                                 : sizeof(struct sockaddr_in6),
                      nullptr, 0, nullptr, (LPOVERLAPPED)ctx);
  }

  return bRetVal;
}