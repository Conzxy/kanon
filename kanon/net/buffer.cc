#include "kanon/net/buffer.h"
#include <sys/uio.h>

using namespace kanon;

Buffer::Buffer(size_type init_size)
  : read_index_{ kBufferPrefixSize }
  , write_index_{ kBufferPrefixSize }
{
  data_.resize(kBufferPrefixSize + init_size);
  static_assert(kBufferPrefixSize == 8, "Buffer prefix size must be 8");
  assert(GetReadableSize() == 0 && "Buffer init readable_size must be 0");
}

Buffer::~Buffer() = default;

Buffer::size_type
Buffer::ReadFd(int fd, int& saved_errno) {
  char extra_buf[65536]; // 64k

  struct iovec vec[2];
  vec[0].iov_base = &*offset(write_index_);
  vec[0].iov_len = GetWritableSize();

  vec[1].iov_base = extra_buf;
  vec[1].iov_len = sizeof extra_buf;
  
  // if GetWritableSize() less than extra_buf, we use two block, then
  // we can read 128k-1 at most.
  // otherwise, we can read GetWritableSize() at most
  // In fact, I think we shouldn't need so big block to read data.
  size_t vec_count = GetWritableSize() < sizeof extra_buf ? 2 : 1;

  auto cache_writable_size = GetWritableSize();

  auto readen_bytes = ::readv(fd, vec, vec_count);

  if (readen_bytes < 0) {
    saved_errno = errno;
  } else if (static_cast<size_type>(readen_bytes) <= cache_writable_size) {
    write_index_ += readen_bytes;  
  } else {
    write_index_ = data_.size();
    Append(extra_buf, readen_bytes - cache_writable_size);
  }

  return readen_bytes;
}

void
Buffer::Shrink(size_type n) {
#if defined(CXX_STANDARD_11) && defined(SELF_DEFINED_SHRINK)
  data_.shrink_to_fit();
#else
  Buffer tmp;
  tmp.MakeSpace(GetReadableSize() + n);
  tmp.Append(ToStringView());
  swap(tmp);
#endif
}
