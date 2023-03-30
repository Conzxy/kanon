#include "kanon/net/chunk_list.h"
#include "kanon/net/tcp_connection.h"
#include "kanon/net/channel.h"
#include "kanon/win/net/completion_context.h"

#include <winsock2.h>
#include <Windows.h>

#include <vector>

using namespace kanon;

#if 0
static void CALLBACK send_completion_callback(DWORD error, DWORD transferred,
                                              LPWSAOVERLAPPED overlapped,
                                              DWORD flags)
{
  auto conn = (TcpConnection *)overlapped;
  auto ch = conn->channel();
  // ch->SetRevents(ch->GetRevents() | Event::ReadEvent);
}
#endif

static CompletionContext *cached_completion_context = nullptr;

void kanon::ChunkListOverlapSend(ChunkList &buffer, FdType fd, int &saved_errno,
                                 void *overlap)
{
  LOG_TRACE << "ChunkListOverlapSend, fd = " << fd;
  auto conn = (TcpConnection *)overlap;
  auto ch = conn->channel();
  auto &bufs = conn->wsabufs;
  bufs.resize(buffer.GetChunkSize());
  auto first_chunk_iter = buffer.begin();
  for (auto &buf : bufs) {
    buf.buf = first_chunk_iter->GetReadBegin();
    buf.len = first_chunk_iter->GetReadableSize();
  }

  DWORD flags = 0;
  DWORD send_bytes = 0;

  CompletionContext *completion_ctx = new CompletionContext;
  completion_context_init(completion_ctx);
  completion_ctx->event = Event::WriteEvent;
  ch->RegisterCompletionContext(completion_ctx);
  // cached_completion_context = completion_ctx;
  auto ret = WSASend(fd, bufs.data(), bufs.size(), &send_bytes, flags,
                     (LPWSAOVERLAPPED)completion_ctx, NULL);
  switch (ret) {
    case 0: // Complete immediately
    {
      LOG_TRACE << "WSASend() complete immediately";
      // conn->HandleWriteImmediately(send_bytes);
    } break;
    case SOCKET_ERROR: {
      auto err = WSAGetLastError();
      if (err == ERROR_IO_PENDING) {
        cached_completion_context = nullptr;
      } else {
        conn->ForceClose();
        LOG_SYSERROR << "Failed to call WSASend()";
      }
    } break;

    default:
      LOG_SYSFATAL << "Unexpected error for calling WSASend()";
  }
}

ChunkList::SizeType kanon::ChunkListWriteFd(ChunkList &buffer, FdType fd,
                                            int &saved_errno) KANON_NOEXCEPT
{
  LOG_FATAL << "ChunkListWriteFd() isn't implemented for Windows";
  return (-1);
}
