#ifndef KANON_BUFFER_RING_BUFFER_H
#define KANON_BUFFER_RING_BUFFER_H

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <vector>

#include "kanon/string/string_view.h"

namespace kanon {

/**
 * \brief A simple ring buffer
 *
 * Ring buffer is designed to be fixed region whose size must be
 * power of 2 to simplify the module operator.
 * To some specific scenario, this is ok and good.
 * \note
 *   This is not used in kanon currently
 */
class RingBuffer {
public:
  using pointer = char*;
  using const_pointer = char const*;
  using data_type = pointer;

  explicit RingBuffer(size_t n);
  ~RingBuffer() noexcept;

  void Prepend(void const* data, size_t len) noexcept;
  void Append(void const* data, size_t len) noexcept;

  void Append(StringView data) noexcept { Append(data.data(), data.size()); }
  void Prepend(StringView data) noexcept { Prepend(data.data(), data.size()); }

  void AdvanceRead(size_t n) noexcept;

  size_t GetRealReadIndex() const noexcept { return read_index_ & GetMask(); }
  size_t GetRealWriteIndex() const noexcept { return write_index_ & GetMask(); }
  pointer GetReadBegin() const noexcept { return data_ + (read_index_ & GetMask()); }
  pointer GetWriteBegin() const noexcept { return data_ + (write_index_ & GetMask()); }
  size_t GetReadableSize() const noexcept { return write_index_ - read_index_; }
  size_t GetWritableSize() const noexcept { return size_ - GetReadableSize(); }
  data_type GetData() const noexcept { return data_; }
  bool HasReadable() const noexcept { return GetReadableSize() == 0; }
  size_t GetMaxSize() const noexcept { return size_; }
  bool IsFull() noexcept { return write_index_ - read_index_ == size_; }

  std::string RetrieveAsString(size_t len);
  std::vector<char> RetrieveAsVector(size_t len);

  std::string RetrieveAllAsString() { return RetrieveAsString(GetReadableSize()); }
  std::vector<char> RetrieveAllAsVector() { return RetrieveAsVector(GetReadableSize()); }

private:
  size_t Write2TailSize() noexcept
  { return size_ - GetRealWriteIndex(); }

  size_t GetMask() const noexcept
  { return (size_ - 1); }

  void Write(void const* data, size_t len) noexcept
  { ::memcpy(GetWriteBegin(), data, len); }

  void AssertDifference() noexcept;

  data_type data_;
  size_t size_;
  size_t read_index_;
  size_t write_index_;
};

} // namespace kanon

#endif // KANON_BUFFER_RING_BUFFER_H