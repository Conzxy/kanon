#ifndef KANON_LEXICAL_STREAM_H
#define KANON_LEXICAL_STREAM_H

#include <string.h>
#include <string>
#include <assert.h>

#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"

#include "kanon/string/string_view.h"
#include "kanon/string/stream_common.h"
#include "kanon/string/fixed_buffer.h"

namespace kanon{

template<unsigned SZ>
class LexicalStream : noncopyable
{
  using Self = LexicalStream;
  using Buffer = detail::FixedBuffer<SZ>;
public:

  LexicalStream() = default;
  LexicalStream(LexicalStream&&) noexcept;
  LexicalStream& operator=(LexicalStream&&) noexcept;  

  void Append(char const* buf, unsigned len) noexcept 
  { buffer_.Append(buf, len); }
  
  void reset() noexcept
  { buffer_.reset(); }

  char const* data() const noexcept
  { return buffer_.data(); }
  
  Buffer& buffer() noexcept
  { return buffer_; }
  
  unsigned size() const noexcept
  { return buffer_.len(); }
  
  unsigned maxsize() const noexcept
  { return SZ; }

  inline Self& operator<<(bool);
  inline Self& operator<<(char);

  inline Self& operator<<(short);
  inline Self& operator<<(unsigned short);
  inline Self& operator<<(int);
  inline Self& operator<<(unsigned);
  inline Self& operator<<(long);
  inline Self& operator<<(unsigned long);
  inline Self& operator<<(long long);
  inline Self& operator<<(unsigned long long);
  inline Self& operator<<(unsigned char);

  Self& operator<<(float f)
  { return *this << static_cast<double>(f); }

  inline Self& operator<<(double);
  
  inline Self& operator<<(char const*);
  inline Self& operator<<(std::string const& str);
  inline Self& operator<<(StringView);

  inline Self& operator<<(void const*);
private:
  Buffer buffer_;
};

/**
 * To inline, we don't put them to source file
 */
template<unsigned SZ>
LexicalStream<SZ>::LexicalStream(LexicalStream&& other) noexcept
  : buffer_(other.buffer())
{ }

template<unsigned SZ>
LexicalStream<SZ>& LexicalStream<SZ>::operator=(LexicalStream&& other) noexcept
{
  buffer_.swap(other.buffer_);  
  return *this;
}

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(char c)->
  Self&
{
  Append(&c, 1);
  return *this;
}

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(bool b)->
  Self&
{
  buffer_.appendBool(b);
  return *this;
}

#define LEXICALSTREAM_OPERATOR_LEFT_SHIFT(type) \
template<unsigned SZ> \
auto LexicalStream<SZ>::operator<<(type i)->\
  Self&\
{\
  buffer_.appendInt(i);\
  return *this; \
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

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(double d)->
  Self&
{
  buffer_.appendFloat(d);
  return *this;
}

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(char const* str)->
  Self&
{
  buffer_.Append(str, strlen(str));
  return *this;
}

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(std::string const& str) ->
  Self&
{
  buffer_.Append(str.data(), str.size());
  return *this;
}

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(StringView sv) ->
  Self&
{
  buffer_.Append(sv);
  return *this;
}

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(void const* p) ->
  Self&
{
  buffer_.appendPtr(p);
  return *this;
}

class Fmt
{
public:
  template<typename T>
  explicit Fmt(char const* str, T val)
  {
    static_assert(std::is_arithmetic<T>::value, 
        "Fmt: Give value's type must be integer or floating-point");
    snprintf(buf_, sizeof buf_, str, val);
    len_ = ::strlen(buf_);
  }

  explicit Fmt(std::string const& str)
    : Fmt(str.c_str(), str.size()) 
  { }
  
  char const* buf() const noexcept
  { return buf_; }

  unsigned len() const noexcept
  { return len_; }

private:
  char buf_[32];
  unsigned len_;
};

template<unsigned SZ>
LexicalStream<SZ>& operator<<(LexicalStream<SZ>& stream, Fmt const& fmt)
{
  stream.Append(fmt.buf(), fmt.len());
  return stream;
}



} // namespace kanon

#endif // KANON_LEXICAL_STREAM_H
