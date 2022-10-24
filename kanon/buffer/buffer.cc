#include "buffer.h"

#include <sys/uio.h>

using namespace kanon;

Buffer::Buffer(size_type init_size)
  : read_index_{ BUFFER_PREFIX_SIZE }
  , write_index_{ BUFFER_PREFIX_SIZE }
{
  data_.Grow(BUFFER_PREFIX_SIZE+init_size);
  static_assert(BUFFER_PREFIX_SIZE == 8, "Buffer prefix size must be 8");
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

  // To ensure the size of read buffer be 64k at least 
  // If the WritableSize() >= 64k, no need use extra_buf,
  // avoid copy extra buffer to kernel space
  // 
  // If writable size is sizeof(extra_buf) - 1, we can read 128k-1 at most
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
  // Buffer tmp;
  // tmp.MakeSpace(GetReadableSize() + n);
  // tmp.Append(ToStringView());
  // swap(tmp);
  if (read_index_ != BUFFER_PREFIX_SIZE)
  {
    std::copy(offset(read_index_), offset(write_index_),
              offset(BUFFER_PREFIX_SIZE));
  }

#ifndef NDEBUG
  if (read_index_ == write_index_) {
    assert(read_index_ == BUFFER_PREFIX_SIZE);
  }
#endif

  data_.Shrink(GetReadableSize() + n);
}

void
Buffer::MakeSpace(size_type len) {
  if (len <= GetWritableSize()) {
    return ;
  }

  // If all unused space size < len + prefix size,
  // we have to expand buffer.
  // Otherwise, we can modify the layout of buffer:
  // Reuse the space between BUFFER_PREFIX_SIZE and read_index
  // +----------+-+---------------+
  // |          | |               |
  // +----------+-+---------------+
  //            ||
  //            VV
  // +-+-+------------------------+
  // | | |                        |
  // +-+-+------------------------+
  //
  if (len >  GetPrependableSize() - BUFFER_PREFIX_SIZE + GetWritableSize()) {
    // FIXME Need copy [readable region] to the BUFFER_PREFIX_SIZE?
  #if 0
    size_t at_least_size = write_index_ + len;
    data_.Grow((at_least_size >= data_.size()) ? at_least_size : data_.size() * 2);
  #else
    data_.Grow(write_index_ + len);
  #endif
    
    // if (GetPrependableSize() < BUFFER_PREFIX_SIZE) {
    //   // It should be discarded
    //   auto r = read_index_ + BUFFER_PREFIX_SIZE - GetPrependableSize(); 
    //   std::copy(offset(r), offset(r+GetReadableSize()), 
    //       offset(BUFFER_PREFIX_SIZE));
    // }
  } else {
    // FIXME The assert is needed?
    KANON_ASSERT(
      BUFFER_PREFIX_SIZE <= read_index_,
     "read_index_ should after prefix size"
     "i.e. message size should prepend at last");

    auto readable = GetReadableSize();

    std::copy(offset(read_index_),
              offset(write_index_),
              offset(BUFFER_PREFIX_SIZE));
    read_index_ = BUFFER_PREFIX_SIZE;
    write_index_ = read_index_ + readable;
    
    assert(readable == GetReadableSize());
  }

  assert(GetWritableSize() >= len);
}
