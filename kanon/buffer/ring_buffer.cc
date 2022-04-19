#include "ring_buffer.h"

#include <stdlib.h>
#include <stdexcept>
#include <assert.h>
#include <string.h>
#include <string>

#include "kanon/util/macro.h"

namespace kanon {

inline static size_t RoundUpToPower2(size_t n) noexcept
{
  if (n == 0) { return 0; }
  if (n == 1) { return 2; }

  size_t res = 1;
  while (res < n) {
    res = res << 1;
  }

  return res;
}

inline static bool IsPower2(size_t n) noexcept
{
  // To check a integer whether satisfy power of 2,
  // we can use (n & (n-1)) since it must have only
  // one 1 bit in its bit represent.
  return ((n != 0) && !(n & (n - 1))) || (n == 0);
}

RingBuffer::RingBuffer(size_t n)
  : size_(RoundUpToPower2(n))
  , read_index_(0)
  , write_index_(0)
{
  KANON_ASSERT(IsPower2(size_), "The size of RingBuffer must be power of 2");
  data_ = (char*)::malloc(size_);

  if (data_ == NULL) {
    throw std::bad_alloc{};
  }
}

RingBuffer::~RingBuffer() noexcept
{
  ::free(data_);
}

void RingBuffer::Append(void const* data, size_t len) noexcept
{
  // Overwrite regardless of it is full
  auto append_size = std::min(len, size_ - GetRealWriteIndex());

  Write(data, append_size);
  write_index_ += append_size;

  if (append_size != len) {
    Write((char const*)data + append_size , len - append_size);
    write_index_ += len - append_size;

  }

  if (write_index_ > read_index_ + size_) {
    read_index_ = write_index_ - size_;
  }

  AssertDifference();
}

void RingBuffer::Prepend(void const* data, size_t len) noexcept
{

}

void RingBuffer::AdvanceRead(size_t len) noexcept
{

}

std::string RingBuffer::RetrieveAsString(size_t len)
{
  assert(len <= GetReadableSize());

  if (GetRealWriteIndex() >= GetRealReadIndex() && !IsFull()) {
    return std::string(GetReadBegin(), len);
  }
  
  std::string res;
  res.reserve(GetReadableSize());

  auto append_size = std::min(len, size_ - GetRealWriteIndex());

  res.append(GetReadBegin(), append_size);

  if (append_size != len) {
    res.append(data_, GetRealWriteIndex());
  }

  return res;
}

std::vector<char> RingBuffer::RetrieveAsVector(size_t len)
{
  assert(len <= GetReadableSize());

  if (GetRealWriteIndex() >= GetRealReadIndex() && !IsFull()) {
    return std::vector<char>(GetReadBegin(), GetReadBegin() + len);
  }
  
  std::vector<char> res;
  res.reserve(GetReadableSize());

  auto append_size = std::min(len, size_ - GetRealWriteIndex());

  res.insert(res.end(), GetReadBegin(), data_ + append_size);

  if (append_size != len) {
    res.insert(res.end(), data_, GetWriteBegin());
  }

  return res;

}

void RingBuffer::AssertDifference() noexcept
{
  KANON_ASSERT(
    write_index_ - read_index_ <= size_,
    "The difference between read_index and write_index should less than size_");
}

} // namespace kanon