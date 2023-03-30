#include "buffer.h"

using namespace kanon;

Buffer::Buffer(size_type init_size)
  : read_index_{BUFFER_PREFIX_SIZE}
  , write_index_{BUFFER_PREFIX_SIZE}
{
  data_.Grow(BUFFER_PREFIX_SIZE + init_size);
  static_assert(BUFFER_PREFIX_SIZE == 8, "Buffer prefix size must be 8");
  assert(GetReadableSize() == 0 && "Buffer init readable_size must be 0");
}

Buffer::~Buffer() = default;

void Buffer::Shrink(size_type n)
{
  // Buffer tmp;
  // tmp.ReserveWriteSpace(GetReadableSize() + n);
  // tmp.Append(ToStringView());
  // swap(tmp);
  if (read_index_ != BUFFER_PREFIX_SIZE) {
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

void Buffer::ReserveWriteSpace(size_type len) KANON_NOEXCEPT
{
  if (len <= GetWritableSize()) {
    return;
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
  if (len > GetPrependableSize() - BUFFER_PREFIX_SIZE + GetWritableSize()) {
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
    KANON_ASSERT(BUFFER_PREFIX_SIZE <= read_index_,
                 "read_index_ should after prefix size"
                 "i.e. message size should prepend at last");

    auto readable = GetReadableSize();

    std::copy(offset(read_index_), offset(write_index_),
              offset(BUFFER_PREFIX_SIZE));
    read_index_ = BUFFER_PREFIX_SIZE;
    write_index_ = read_index_ + readable;

    assert(readable == GetReadableSize());
  }

  assert(GetWritableSize() >= len);
}
