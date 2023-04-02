#include "kanon/net/buffer.h"
#include "kanon/log/logger.h"

#include <sys/uio.h>

using namespace kanon;

usize kanon::BufferReadFromFd(Buffer &buffer, FdType fd, int &saved_errno)
{
  char extra_buf[65536]; // 64k

  struct iovec vec[2];
  vec[0].iov_base = buffer.GetWriteBegin();
  vec[0].iov_len = buffer.GetWritableSize();

  vec[1].iov_base = extra_buf;
  vec[1].iov_len = sizeof extra_buf;

  // To ensure the size of read buffer be 64k at least
  // If the WritableSize() >= 64k, no need use extra_buf,
  // avoid copy extra buffer to kernel space
  //
  // If writable size is sizeof(extra_buf) - 1, we can read 128k-1 at most
  size_t vec_count = buffer.GetWritableSize() < sizeof extra_buf ? 2 : 1;

  auto cache_writable_size = buffer.GetWritableSize();

  auto readen_bytes = ::readv(fd, vec, (int)vec_count);

  if (readen_bytes < 0) {
    saved_errno = errno;
  } else if (static_cast<Buffer::size_type>(readen_bytes) <=
             cache_writable_size) {
    buffer.AdvanceWrite((size_t)readen_bytes);
  } else {
    buffer.SetWriteIndex(buffer.GetCapacity());
    buffer.Append(extra_buf, (size_t)readen_bytes - cache_writable_size);
  }

  return (size_t)readen_bytes;
}

void kanon::BufferOverlapRecv(Buffer &, FdType, int &, void *)
{
  LOG_FATAL << "BufferOverlapRecv() isn't implemented for Linux";
}
