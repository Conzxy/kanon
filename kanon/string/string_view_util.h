#ifndef KANON_STRING_VIEW_UTIL_H__
#define KANON_STRING_VIEW_UTIL_H__

#include "string_view.h"
#include "kanon/util/optional.h"

namespace kanon {

/**
 * strtoull() wrapper
 *
 * To robust, reuse the libc routines
 */
optional<uint64_t> StringViewToU64(StringView const &sv, int base=10) noexcept;

/**
 * strtoul() wrapper
 */
optional<uint32_t> StringViewToU32(StringView const &sv, int base=10) noexcept;

/**
 * strtoll() wrapper
 */
optional<int64_t> StringViewTo64(StringView const &sv, int base=10) noexcept;

/**
 * strtol() wrapper
 */
optional<int32_t> StringViewTo32(StringView const &sv, int base=10) noexcept;

inline optional<int16_t> StringViewTo16(StringView const &sv, int base=10) noexcept
{
  return StringViewTo32(sv, base);
}

inline optional<uint16_t> StringViewToU16(StringView const &sv, int base=10) noexcept
{
  return StringViewToU32(sv, base);
}

inline optional<int8_t> StringViewTo8(StringView const &sv, int base=10) noexcept
{
  return StringViewTo32(sv, base);
}

inline optional<uint8_t> StringViewToU8(StringView const &sv, int base=10) noexcept
{
  return StringViewToU32(sv, base);
}

} // namespace kanon

#endif // KANON_STRING_VIEW_UTIL_H__
