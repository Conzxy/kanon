#include "lexical_stream.h"

namespace kanon {

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

} // namespace kanon