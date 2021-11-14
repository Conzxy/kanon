#ifndef KANON_LEXICAL_CAST_H
#define KANON_LEXICAL_CAST_H

#include "lexical_stream.h"
#include <string>

namespace kanon{
  
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
    return (SmallLexicalStream{} << src).data(); \
  } \
};

DST_STRING_SPECIALIZATION(char const*)
DST_STRING_SPECIALIZATION(std::string)

template<typename Dst, typename Src>
Dst lexical_cast(Src const& src)
{ return LexicalCast<Dst, Src>::apply(src); }

} // namespace kanon

#endif // KANON_LEXICAL_CAST_H
