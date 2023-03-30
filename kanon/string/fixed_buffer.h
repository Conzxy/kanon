#ifndef KANON_STRING_FIXED_BUFFER_H
#define KANON_STRING_FIXED_BUFFER_H

#include <stdint.h>
#include <stdio.h>
#include <type_traits>
#include <algorithm> // std::reverse

#include "kanon/util/macro.h"

#include "kanon/string/stream_common.h"
#include "kanon/string/string_view.h"

namespace kanon {
namespace detail {

template <typename T,
          typename = typename std::enable_if<std::is_integral<T>::value>::type>
unsigned int2Str(char *buf, T integer);

KANON_CORE_API unsigned ptrToHexStr(char *buf, uintptr_t p);

/**
 * \tparam SZ size of buffer
 * Presents a fixed-size buffer
 *
 * This is an internal class
 */
template <unsigned SZ>
class FixedBuffer {
  using Self = FixedBuffer;

 public:
  FixedBuffer()
    : len_(0)
  {
  }

  // prohibit modify througt data()
  char const *data() const KANON_NOEXCEPT { return data_; }

  // length, avaliable space
  unsigned len() const KANON_NOEXCEPT { return len_; }

  StringView ToStringView() const KANON_NOEXCEPT { return StringView(data_, len_); }

  bool empty() const KANON_NOEXCEPT { return len() == 0; }

  unsigned avali() const KANON_NOEXCEPT { return SZ - len_; }

  char const *end() const KANON_NOEXCEPT { return data_ + SZ; }

  // append
  void Append(char const *str, unsigned len) KANON_NOEXCEPT
  {
    if (len < avali()) {
      memcpy(cur(), str, len);
      len_ += len;
      *cur() = 0;
    }
  }

  void Append(StringView str) KANON_NOEXCEPT { Append(str.data(), str.size()); }

  template <typename T>
  void appendInt(T i) KANON_NOEXCEPT
  {
    if (avali() > kMaxIntSize) {
      AdvanceRead(detail::int2Str(cur(), i));
      *cur() = 0;
    }
  }

  void appendBool(bool b) KANON_NOEXCEPT
  {
    if (b) {
      Append("True", 4);
    } else {
      Append("False", 5);
    }
  }

  void appendChar(char c) KANON_NOEXCEPT { Append(&c, 1); }

  void appendFloat(double d) KANON_NOEXCEPT
  {
    if (avali() > kMaxFloatingSize) {
      AdvanceRead(::snprintf(cur(), kMaxFloatingSize, "%.12g", d));
      *cur() = 0;
    }
  }

  void appendPtr(void const *p) KANON_NOEXCEPT
  {
    auto up = reinterpret_cast<uintptr_t>(p);
    auto cur = this->cur();

    if (!p && avali() > 6) {
      Append("(null)", 6);
    } else if (avali() > 10) {
      cur[0] = '0';
      cur[1] = 'x';

      AdvanceRead(detail::ptrToHexStr(cur + 2, up) + 2);
      *this->cur() = 0;
    }
  }

  // inplace modify
  // cur() -> set()
  char *cur() KANON_NOEXCEPT { return data_ + len_; }

  void reset() KANON_NOEXCEPT { len_ = 0; }

  void AdvanceRead(unsigned diff) KANON_NOEXCEPT
  {
    len_ += diff;
    assert(len_ < SZ);
  }

  void swap(Self &other) KANON_NOEXCEPT
  {
    std::swap(data_, other.data_);
    std::swap(len_, other.len_);
  }

  void zero() KANON_NOEXCEPT { ::memset(data_, 0, SZ); }

 private:
  static constexpr unsigned kMaxIntSize = 32;
  static constexpr unsigned kMaxFloatingSize = 324;

  char data_[SZ];
  unsigned len_;
};

// template<unsigned SZ>
// constexpr unsigned FixedBuffer<SZ>::kMaxFloatingSize;

// template<unsigned SZ>
// constexpr unsigned FixedBuffer<SZ>::kMaxIntSize;

template <unsigned SZ>
void swap(FixedBuffer<SZ> &lhs,
          FixedBuffer<SZ> &rhs) KANON_NOEXCEPT_OP(KANON_NOEXCEPT_OP(lhs.swap(rhs)))
{
  lhs.swap(rhs);
}

template <typename T, typename>
unsigned int2Str(char *buf, T integer)
{
  static char const digits[] = "9876543210123456789";
  static_assert(sizeof digits == 20, "digits size should be 20");

  static char const *pzero = digits + 9;

  char *end = buf;

  bool negative = integer < 0;

  do {
    T left = integer % 10;
    integer /= 10;
    *(end++) = *(pzero + left);
  } while (integer != 0);

  if (negative) *(end++) = '-';

  *end = 0;
  std::reverse(buf, end);

  return end - buf;
}

} // namespace detail
} // namespace kanon

#endif // KANON_STRING_FIXED_BUFFER_H
