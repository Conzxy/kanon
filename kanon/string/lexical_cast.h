#ifndef KANON_LEXICAL_CAST_H
#define KANON_LEXICAL_CAST_H

#include <string.h>
#include <string>

#include "kanon/util/optional.h"
#include "kanon/util/arithmetic_type.h"

#include "kanon/string/lexical_stream.h"
#include "kanon/string/type.h"

namespace kanon {
  
template<
  typename Dst, 
  typename Src, 
  bool = std::is_same<Dst, Src>::value>
class LexicalCast
{
public:
  static void apply(Src const& ) 
  {
    static_assert(sizeof(Src) < 0,
      "lexical_cast only support string-to-numric or numric-to-string");
  }
};

#define DST_STRING_SPECIALIZATION(type) \
template<typename Src> \
class LexicalCast<type, Src, false> \
{ \
public: \
  static type apply(Src const& src) \
  { \
    static CastLexicalStream stream; \
    stream.reset(); \
    return (stream << src).data(); \
  } \
};

DST_STRING_SPECIALIZATION(char const*)
DST_STRING_SPECIALIZATION(std::string)
DST_STRING_SPECIALIZATION(kanon::StringView)

#define Str2Int(integer_type, str_type, func) \
template<> \
class LexicalCast<integer_type, str_type, false> { \
public: \
  static kanon::optional<integer_type> apply(str_type const& src) { \
    static char buf[64]; \
    ::strncpy(buf, src.data(), src.size()); \
    buf[src.size()] = 0; \
 \
    char* endptr; \
 \
    auto ret = static_cast<integer_type>(func(buf, &endptr, 10)); \
    if (*endptr == 0) { \
      return kanon::optional<integer_type>(ret); \
    }   \
 \
    return kanon::make_null_optional<long>(); \
  }  \
};

// i32 == int
// i64 == long
// isize = long
// u~ == ~
#define Str2IntForOne(str) \
Str2Int(i8, str, ::strtol) \
Str2Int(i16, str, ::strtol) \
Str2Int(int,                str, ::strtol) \
Str2Int(long,               str, ::strtol) \
Str2Int(long long,          str, ::strtoll) \
Str2Int(u8, str, ::strtoul) \
Str2Int(u16, str, ::strtoul) \
Str2Int(unsigned int,       str, ::strtoul) \
Str2Int(unsigned long,      str, ::strtoul) \
Str2Int(unsigned long long, str, ::strtoull) \

Str2IntForOne(kanon::StringView)
Str2IntForOne(std::string)

template<typename Dst, typename Src>
auto lexical_cast(Src const& src)
  -> decltype(LexicalCast<Dst, Src>::apply(src))
{ return LexicalCast<Dst, Src>::apply(src); }

} // namespace kanon

#endif // KANON_LEXICAL_CAST_H
