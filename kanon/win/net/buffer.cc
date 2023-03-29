#include "kanon/net/buffer.h"
#include "kanon/log/logger.h"

#include <winsock2.h>
#include "kanon/net/tcp_connection.h"
#include "kanon/net/channel.h"

#include "kanon/win/net/completion_context.h"

using namespace kanon;

#if 0
static void CALLBACk recv_completion_callback(DWORD error, DWORD transferred,
                                     LPWSAOVERLAPPED overlapped, DWORD flags)
{
  auto conn = (TcpConnection *)overlapped;
  auto ch = conn->channel();
  // ch->SetRevents(ch->GetRevents() | Event::ReadEvent);
}
#endif

static CompletionContext *cached_completion_context = nullptr;

void kanon::BufferOverlapRecv(Buffer &buffer, FdType fd, int &saved_errno,
                              void *overlap)
{
  LOG_TRACE << "BufferOverlapRecv(), fd = " << fd;

  auto conn = (TcpConnection *)overlap;
  auto ch = conn->channel();
  auto &bufs = conn->wsabufs;
  // FIXME Should shrink bufs?
  if (bufs.size() < 1) bufs.resize(1);

  bufs[0].buf = buffer.GetWriteBegin();
  bufs[0].len = buffer.GetWritableSize();
  DWORD flags = 0;
  DWORD recv_bytes;
  CompletionContext *completion_ctx = new CompletionContext;
  completion_context_init(completion_ctx);
  completion_ctx->event = Event::ReadEvent;
  ch->RegisterCompletionContext(completion_ctx);
  // cached_completion_context = completion_ctx;
  auto ret = WSARecv(fd, bufs.data(), 1, &recv_bytes, &flags,
                     (LPWSAOVERLAPPED)completion_ctx, NULL);

  switch (ret) {
    case 0: // Complete immediately
    {
      LOG_TRACE << "WSARecv() complete immediately";
      // conn->HandleReadImmediately(recv_bytes);
    } break;

    case SOCKET_ERROR: {
      auto err = WSAGetLastError();
      if (err != WSA_IO_PENDING) {
        LOG_SYSERROR << "Failed to call WSARecv()";
      } else {
        cached_completion_context = nullptr;
      }
    } break;
  }
}

usize kanon::BufferReadFromFd(Buffer &buffer, FdType fd, int &saved_errno)
{
  LOG_FATAL << "BufferReadFromFd() isn't implemented for Windows";
  return usize(-1);
}
