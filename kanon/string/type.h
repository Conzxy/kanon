#ifndef KANON_STRING_TYPE_H
#define KANON_STRING_TYPE_H

#include "kanon/string/fixed_buffer.h"
#include "kanon/string/fmt_stream.h"
#include "kanon/string/lexical_stream.h"

namespace kanon {
namespace detail {

template<unsigned SZ>
class FixedBuffer;

using SmallFixedBuffer = FixedBuffer<kSmallStreamSize>;
using LargeFixedBuffer = FixedBuffer<kLargeStreamSize>;

} // namespace detail

template<unsigned SZ>
class LexicalStream;

using SmallLexicalStream = LexicalStream<kSmallStreamSize>;
using LargeLexicalStream = LexicalStream<kLargeStreamSize>;
using LogLexicalStream = SmallLexicalStream;

using CastLexicalStream = LexicalStream<kCastStreamSize>;

template<unsigned SZ>
class FmtStream;

using SmallFmtStream = FmtStream<kSmallStreamSize>;
using LargeFmtStream = FmtStream<kLargeStreamSize>;
using LogFmtStream = SmallFmtStream;

} // namespace kanon

#endif 