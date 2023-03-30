#ifndef KANON_STRING_FMT_STREAM_H
#define KANON_STRING_FMT_STREAM_H

#include <assert.h>

#include "kanon/string/stream_common.h"
#include "kanon/string/fixed_buffer.h"

namespace kanon {

/**
 * Since format string need to parse, the time cost
 * is larger than the LexicalStream.
 * To Log, the bette choice is LexicalStream
 * althouth maybe not be beatiful for format output.
 */
template<unsigned SZ>
class FmtStream {
  using Stream = detail::FixedBuffer<SZ>;
public:
  FmtStream() = default;
  /**
   * I don't write "Args&&..." since we just accept builtin type
   * then value-passed argument can stored in register instead in memory
   * But this is not significant in release mode
   */
  template<typename ...Args>
  KANON_INLINE FmtStream(StringView fmt, Args... args);

  template<typename ...Args>
  KANON_INLINE void Serialize(StringView fmt, Args... args);

  char const* GetData() const KANON_NOEXCEPT { return stream_.data(); }
  void Reset() KANON_NOEXCEPT { stream_.reset(); }
  StringView ToStringView() const KANON_NOEXCEPT { return stream_.ToStringView(); }
private:
  template<typename ...Args>
  KANON_INLINE void ParseAndSerialize(StringView fmt, Args... args);

  void ParseAndSerialize(StringView fmt);

  // PostCondition: at least one argument
  template<typename Arg, typename ...Args>
  void ParseAndSerializeSingle(StringView fmt, Arg arg, Args... args);

  template<typename T, 
   typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  void SerializeOne(T arg) KANON_NOEXCEPT { stream_.appendInt(arg); }

  template<typename T,
    typename std::enable_if<std::is_floating_point<T>::value, char>::type = 0>
  void SerializeOne(T arg) KANON_NOEXCEPT { stream_.appendFloat(static_cast<double>(arg)); }

  void SerializeOne(bool arg) KANON_NOEXCEPT { stream_.appendBool(arg); }
  void SerializeOne(void const* p) KANON_NOEXCEPT { stream_.appendPtr(p); }
  void SerializeOne(char c) KANON_NOEXCEPT { stream_.appendChar(c); }

  detail::FixedBuffer<SZ> stream_;
};

#define FMT_STREAM_TEMPLATE FmtStream<SZ>
#define TEMPLATE_OF_FMT_STREAM template<unsigned SZ>

/**
 * Because the function template argument is unknown,
 * I can't put them to source file
 */
TEMPLATE_OF_FMT_STREAM
template<typename ...Args>
FMT_STREAM_TEMPLATE::FmtStream(StringView fmt, Args... args)
  : stream_()
{ ParseAndSerialize(fmt, args...); }

TEMPLATE_OF_FMT_STREAM
template<typename ...Args>
void FMT_STREAM_TEMPLATE::ParseAndSerialize(StringView fmt, Args... args) 
{ ParseAndSerializeSingle(fmt, args...); }

TEMPLATE_OF_FMT_STREAM
template<typename ...Args>
void FMT_STREAM_TEMPLATE::Serialize(StringView fmt, Args... args) 
{ ParseAndSerialize(fmt, args...); }

TEMPLATE_OF_FMT_STREAM
void FMT_STREAM_TEMPLATE::ParseAndSerialize(StringView fmt)
{
  auto pos = fmt.find('%');

  while (pos < fmt.size()) {
    KANON_ASSERT(
      pos != fmt.size() - 1 && fmt[pos+1] == '%',
      "If you want print %, should use %%."
      "Otherwise, the number of argument is less than the % placeholder");

    stream_.Append(fmt.substr(0, pos+1));
    fmt.remove_prefix(pos+2);
    pos = fmt.find('%');
  }

  stream_.Append(fmt);    
}

TEMPLATE_OF_FMT_STREAM
template<typename Arg, typename ...Args>
void FMT_STREAM_TEMPLATE::ParseAndSerializeSingle(StringView fmt, Arg arg, Args... args)
{
  // Parse
  const auto pos = fmt.find("%");
  
  stream_.Append(fmt.data(), pos == StringView::npos ? fmt.size() : pos);

  if (pos < fmt.size()) {
    if (fmt[pos+1] == '%') {
      stream_.Append("%");
      ParseAndSerialize(fmt.substr(pos+2), arg, args...);
    }
    else {
      SerializeOne(arg);
      ParseAndSerialize(fmt.substr(pos+1), args...);
    }
  }
  else {
    KANON_ASSERT(false, "The number of argument is greater than the % placeholder");
  }
} 


} // namespace kanon

#endif // KANON_STRING_FMT_STREAM