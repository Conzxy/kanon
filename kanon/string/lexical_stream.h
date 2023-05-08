#ifndef KANON_LEXICAL_STREAM_H
#define KANON_LEXICAL_STREAM_H

#include <string.h>
#include <string>
#include <assert.h>

#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"

#include "kanon/string/string_view.h"
#include "kanon/string/stream_common.h"
#include "kanon/string/fmt_stream.h"
#include "kanon/string/fixed_buffer.h"

namespace kanon {

template <unsigned SZ>
class LexicalStream : noncopyable {
  using Self = LexicalStream;
  using Buffer = detail::FixedBuffer<SZ>;

 public:
  LexicalStream() = default;
  LexicalStream(LexicalStream &&) KANON_NOEXCEPT;
  LexicalStream &operator=(LexicalStream &&) KANON_NOEXCEPT;

  void Append(char const *buf, unsigned len) KANON_NOEXCEPT
  {
    buffer_.Append(buf, len);
  }

  void reset() KANON_NOEXCEPT { buffer_.reset(); }

  char const *data() const KANON_NOEXCEPT { return buffer_.data(); }

  Buffer &buffer() KANON_NOEXCEPT { return buffer_; }

  unsigned size() const KANON_NOEXCEPT { return buffer_.len(); }

  unsigned maxsize() const KANON_NOEXCEPT { return SZ; }

  KANON_INLINE Self &operator<<(bool);
  KANON_INLINE Self &operator<<(char);

  KANON_INLINE Self &operator<<(short);
  KANON_INLINE Self &operator<<(unsigned short);
  KANON_INLINE Self &operator<<(int);
  KANON_INLINE Self &operator<<(unsigned);
  KANON_INLINE Self &operator<<(long);
  KANON_INLINE Self &operator<<(unsigned long);
  KANON_INLINE Self &operator<<(long long);
  KANON_INLINE Self &operator<<(unsigned long long);
  KANON_INLINE Self &operator<<(unsigned char);

  template <unsigned N>
  KANON_INLINE Self &operator<<(FmtStream<N> const &fmt_stream);

  Self &operator<<(float f) { return *this << static_cast<double>(f); }

  KANON_INLINE Self &operator<<(double);

  KANON_INLINE Self &operator<<(char const *);
  KANON_INLINE Self &operator<<(std::string const &str);
  KANON_INLINE Self &operator<<(StringView);

  KANON_INLINE Self &operator<<(void const *);

 private:
  Buffer buffer_;
};

/**
 * To KANON_INLINE, we don't put them to source file
 */
template <unsigned SZ>
LexicalStream<SZ>::LexicalStream(LexicalStream &&other) KANON_NOEXCEPT
  : buffer_(other.buffer())
{
}

template <unsigned SZ>
LexicalStream<SZ> &
LexicalStream<SZ>::operator=(LexicalStream &&other) KANON_NOEXCEPT
{
  buffer_.swap(other.buffer_);
  return *this;
}

template <unsigned SZ>
auto LexicalStream<SZ>::operator<<(char c) -> Self &
{
  Append(&c, 1);
  return *this;
}

template <unsigned SZ>
auto LexicalStream<SZ>::operator<<(bool b) -> Self &
{
  buffer_.appendBool(b);
  return *this;
}

#define LEXICALSTREAM_OPERATOR_LEFT_SHIFT(type)                                \
  template <unsigned SZ>                                                       \
  auto LexicalStream<SZ>::operator<<(type i)->Self &                           \
  {                                                                            \
    buffer_.appendInt(i);                                                      \
    return *this;                                                              \
  }

LEXICALSTREAM_OPERATOR_LEFT_SHIFT(unsigned char)
LEXICALSTREAM_OPERATOR_LEFT_SHIFT(short)
LEXICALSTREAM_OPERATOR_LEFT_SHIFT(unsigned short)
LEXICALSTREAM_OPERATOR_LEFT_SHIFT(int)
LEXICALSTREAM_OPERATOR_LEFT_SHIFT(unsigned)
LEXICALSTREAM_OPERATOR_LEFT_SHIFT(long)
LEXICALSTREAM_OPERATOR_LEFT_SHIFT(unsigned long)
LEXICALSTREAM_OPERATOR_LEFT_SHIFT(long long)
LEXICALSTREAM_OPERATOR_LEFT_SHIFT(unsigned long long)

template <unsigned SZ>
auto LexicalStream<SZ>::operator<<(double d) -> Self &
{
  buffer_.appendFloat(d);
  return *this;
}

template <unsigned SZ>
auto LexicalStream<SZ>::operator<<(char const *str) -> Self &
{
  buffer_.Append(str, (unsigned)strlen(str));
  return *this;
}

template <unsigned SZ>
auto LexicalStream<SZ>::operator<<(std::string const &str) -> Self &
{
  buffer_.Append(str.data(), (unsigned)str.size());
  return *this;
}

template <unsigned SZ>
auto LexicalStream<SZ>::operator<<(StringView sv) -> Self &
{
  buffer_.Append(sv);
  return *this;
}

template <unsigned SZ>
auto LexicalStream<SZ>::operator<<(void const *p) -> Self &
{
  buffer_.appendPtr(p);
  return *this;
}

template <unsigned SZ>
template <unsigned N>
auto LexicalStream<SZ>::operator<<(FmtStream<N> const &fmt_stream) -> Self &
{
  buffer_.Append(fmt_stream.ToStringView());
  return *this;
}

/**
 * Please use FmtStream to replace this in new code
 */
class KANON_DEPRECATED_ATTR Fmt {
 public:
  using size_type = unsigned int;
  template <typename T>
  explicit Fmt(char const *str, T val)
  {
    static_assert(std::is_arithmetic<T>::value,
                  "Fmt: Give value's type must be integer or floating-point");
    auto ret = snprintf(buf_, sizeof buf_, str, val);
    if (ret < 0) throw std::runtime_error("Failed to call snprintf in Fmt");
    len_ = (size_type)ret;
  }

  explicit Fmt(std::string const &str)
    : Fmt(str.c_str(), str.size())
  {
  }

  char const *buf() const KANON_NOEXCEPT { return buf_; }

  size_type len() const KANON_NOEXCEPT { return len_; }

 private:
  char buf_[32];
  size_type len_;
};

#if 0
template <unsigned SZ>
KANON_DEPRECATED_ATTR LexicalStream<SZ> &operator<<(LexicalStream<SZ> &stream,
                                                    Fmt const &fmt)
{
  stream.Append(fmt.buf(), fmt.len());
  return stream;
}
#endif

} // namespace kanon

#endif // KANON_LEXICAL_STREAM_H
