#include "kanon/net/chunk_list.h"

#include <sys/uio.h>
#include "kanon/algo/fixed_vector.h"
#include "kanon/net/sock_api.h"

using namespace kanon;

#ifdef IOV_MAX
static constexpr unsigned IOVEC_MAX = IOV_MAX;
#else
static constexpr unsigned IOVEC_MAX = 1024;
#endif

static size_t RoundUpDivideIn2(size_t x, size_t y) KANON_NOEXCEPT
{
  return (x == 0) ? 0 : ((x - 1) / y + 1);
}

ChunkList::SizeType kanon::ChunkListWriteFd(ChunkList &buffer, FdType fd,
                                            int &saved_errno) KANON_NOEXCEPT
{
  if (buffer.GetChunkSize() == 1) {
    auto first_chunk = buffer.GetFirstChunk();
    return sock::Write(fd, buffer.GetFirstChunk()->GetReadBegin(),
                       first_chunk->GetReadableSize());
  }

  FixedVector<struct iovec> iovecs(buffer.GetChunkSize());
  auto first_chunk = buffer.begin();
  auto cnt = RoundUpDivideIn2(buffer.GetChunkSize(), IOVEC_MAX);

  int ret = 0;

  while (cnt--) {
    for (auto &x : iovecs) {
      x.iov_base = first_chunk->GetReadBegin();
      x.iov_len = first_chunk->GetReadableSize();
      ++first_chunk;
    }

    auto n = ::writev(fd, iovecs.data(), iovecs.size());

    if (n >= 0) {
      ret += n;
    } else {
      saved_errno = errno;
      break;
    }
  }

  return ret;
}

void kanon::ChunkListOverlapSend(ChunkList &buffer, FdType fd, int &saved_errno,
                                 void *overlap)
{
  LOG_FATAL << "ChunkListOverlapSend() isn't implemented for Linux";
}
