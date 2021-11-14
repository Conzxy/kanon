#ifndef KANON_LEXICAL_STREAM_H
#define KANON_LEXICAL_STREAM_H

#include "kanon/string/string_view.h"
#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"

#include <string.h>
#include <string>
#include <stdint.h>
#include <type_traits>
#include <algorithm>
#include <assert.h>

namespace kanon{

namespace detail{
/**
 * @class FixedBuffer
 * @tparam SZ size of buffer
 * maintain a fixed-size buffer
 */
template<unsigned SZ>
class FixedBuffer
{
  using Self = FixedBuffer;
public:
  FixedBuffer() 
    : len_(0) 
  { }
  
  //prohibit modify througt data()
  char const* data() const KANON_NOEXCEPT { return data_; }
  
  // length, avaliable space
  unsigned len() const KANON_NOEXCEPT 
  { return len_; }

  bool empty() const KANON_NOEXCEPT
  { return len() == 0; }

  unsigned avali() const KANON_NOEXCEPT 
  { return SZ - len_; }
  
  char const* end() const KANON_NOEXCEPT 
  { return data_ + SZ; }

  // append
  void append(char const* str, unsigned len) KANON_NOEXCEPT {
    if(len < avali()) {
      memcpy(cur(), str, len);
      len_ += len;
      *cur() = 0;
    } 
  }
  
  // inplace modify
  // cur() -> set()
  char* cur() KANON_NOEXCEPT
  { return data_ + len_; }

  void reset() KANON_NOEXCEPT
  { len_ = 0; }

  void add(unsigned diff) KANON_NOEXCEPT
  { 
    len_ += diff; 
    assert(len_ < SZ);
  }
  
  void swap(Self& other) KANON_NOEXCEPT{
    std::swap(data_, other.data_);
    std::swap(len_, other.len_);
  }

  void zero() KANON_NOEXCEPT
  { memset(data_, 0, SZ); }
private:
  char data_[SZ];
  unsigned len_;
};

template<unsigned SZ>
void swap(FixedBuffer<SZ> const& lhs, FixedBuffer<SZ> const& rhs) KANON_NOEXCEPT(KANON_NOEXCEPT(lhs.swap(rhs)))
{ lhs.swap(rhs); }

} // namespace detail

template<unsigned SZ>
class LexicalStream : noncopyable
{
  using Self = LexicalStream;
  using Buffer = detail::FixedBuffer<SZ>;
public:

  LexicalStream() = default;
  LexicalStream(LexicalStream&&) KANON_NOEXCEPT;
  LexicalStream& operator=(LexicalStream&&) KANON_NOEXCEPT;  

  void append(char const* buf, unsigned len) KANON_NOEXCEPT 
  { buffer_.append(buf, len); }
  
  void reset() KANON_NOEXCEPT
  { buffer_.reset(); }

  char const* data() const KANON_NOEXCEPT
  { return buffer_.data(); }
  
  Buffer& buffer() KANON_NOEXCEPT
  { return buffer_; }
  
  unsigned size() const KANON_NOEXCEPT
  { return buffer_.len(); }
  
  unsigned maxsize() const KANON_NOEXCEPT
  { return SZ; }

  Self& operator<<(bool);
  Self& operator<<(char);

  Self& operator<<(short);
  Self& operator<<(unsigned short);
  Self& operator<<(int);
  Self& operator<<(unsigned);
  Self& operator<<(long);
  Self& operator<<(unsigned long);
  Self& operator<<(long long);
  Self& operator<<(unsigned long long);
  Self& operator<<(unsigned char);

  Self& operator<<(float f)
  { return *this << static_cast<double>(f); }

  Self& operator<<(double);
  
  Self& operator<<(char const*);
  Self& operator<<(std::string const& str);
  Self& operator<<(StringView const&);

  Self& operator<<(void const*);
private:
  Buffer buffer_;

// static
public:
  static constexpr unsigned kMaxIntSize = 32;
  static constexpr unsigned kMaxFloatingSize = 324;
};

template<unsigned SZ>
constexpr unsigned LexicalStream<SZ>::kMaxFloatingSize;

template<unsigned SZ>
LexicalStream<SZ>::LexicalStream(LexicalStream&& other) KANON_NOEXCEPT
  : buffer_(other.buffer())
{ }

template<unsigned SZ>
LexicalStream<SZ>& LexicalStream<SZ>::operator=(LexicalStream&& other) KANON_NOEXCEPT
{
  buffer_.swap(other.buffer_);  
  return *this;
}

namespace detail {
  
  template<
    typename T,
    typename = typename std::enable_if<
      std::is_integral<T>::value
      >::type>
  unsigned int2Str(char* buf, T integer){
    static char const digits[] = "9876543210123456789";
    static_assert(sizeof digits == 20, "digits size should be 20");

    static char const* pzero = digits + 9;

    char* end = buf;

    do{
      T left = integer % 10;
      integer /= 10;
      *(end++) = *(pzero + left);
    }while(integer != 0);
    
    if(integer < 0)
      *(end++) = '-';
  
    *end = 0;
    std::reverse(buf, end);
    
    return end - buf;
  }

  inline unsigned ptrToHexStr(char* buf, uintptr_t p) {
    static char const hex_digits[] = "0123456789ABCDEF";
    static_assert(sizeof hex_digits == 17, "hex_digits size should be 17");

    auto end = buf;
    auto count = 0;

    do {
      auto r = p % 16;
      p = p / 16;
      *(end++) = hex_digits[r];
      ++count;
    } while (p != 0);

    if (count < 16) {
      int rest = 16 - count;

      for (; rest > 0; --rest)
        *(end++) = '0';
    }

    std::reverse(buf, end);
    *end = 0;

    return end - buf;
  }

} // namespace detail

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(char c)->
  Self&
{
  append(&c, 1);
  return *this;
}

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(bool b)->
  Self&
{
  if(b)
    append("True", 4);
  else
    append("False", 5);
  
  return *this;
}

#define LEXICALSTREAMg_OPERATOR_LEFT_SHIFT(type) \
template<unsigned SZ> \
auto LexicalStream<SZ>::operator<<(type i)->\
  Self&\
{\
  if(buffer_.avali() > kMaxIntSize)\
    buffer_.add(detail::int2Str(buffer_.cur(), i)); \
  return *this; \
}

LEXICALSTREAMg_OPERATOR_LEFT_SHIFT(unsigned char)
LEXICALSTREAMg_OPERATOR_LEFT_SHIFT(short)
LEXICALSTREAMg_OPERATOR_LEFT_SHIFT(unsigned short)
LEXICALSTREAMg_OPERATOR_LEFT_SHIFT(int)
LEXICALSTREAMg_OPERATOR_LEFT_SHIFT(unsigned)
LEXICALSTREAMg_OPERATOR_LEFT_SHIFT(long)
LEXICALSTREAMg_OPERATOR_LEFT_SHIFT(unsigned long)
LEXICALSTREAMg_OPERATOR_LEFT_SHIFT(long long)
LEXICALSTREAMg_OPERATOR_LEFT_SHIFT(unsigned long long)

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(double d)->
  Self&
{
  if(buffer_.avali() > kMaxFloatingSize){
    int len = snprintf(buffer_.cur(), kMaxFloatingSize, "%.12g", d);

    buffer_.add(len);
  }

  return *this;
}

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(char const* str)->
  Self&
{
  if(str){
    append(str, strlen(str));
  }else{
    append("(null)", 6);
  }

  return *this;
}

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(std::string const& str) ->
  Self&
{
  append(str.data(), str.size());
  return *this;
}

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(StringView const& sv) ->
  Self&
{
  append(sv.data(), sv.size());
  return *this;
}

template<unsigned SZ>
auto LexicalStream<SZ>::operator<<(void const* p) ->
  Self&
{
  auto up = reinterpret_cast<uintptr_t>(p);

  auto cur = buffer_.cur();

  if (buffer_.avali() > 18) {
    cur[0] = '0';
    cur[1] = 'x';

    buffer_.add(detail::ptrToHexStr(cur+2, up) + 2);
  }

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
  
  char const* buf() const KANON_NOEXCEPT
  { return buf_; }

  unsigned len() const KANON_NOEXCEPT
  { return len_; }

private:
  char buf_[32];
  unsigned len_;
};

template<unsigned SZ>
LexicalStream<SZ>& operator<<(LexicalStream<SZ>& stream, Fmt const& fmt)
{
  stream.append(fmt.buf(), fmt.len());
  return stream;
}

constexpr int64_t kSmallStreamSize = 4000;
constexpr int64_t kLargeStreamSize = 4000 * 1000;

#define SMALL_FIXEDBUFFER_SIZE 4000
#define LARGE_FIXEDBUFFER_SIZE 4000 * 1000

using SmallLexicalStream = LexicalStream<kSmallStreamSize>;
using LargeLexicalStream = LexicalStream<kLargeStreamSize>;

} // namespace kanon

#endif // KANON_LEXICAL_STREAM_H
