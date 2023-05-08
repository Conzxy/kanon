#ifndef KANON_STRING_VIEW_UTIL_H__
#define KANON_STRING_VIEW_UTIL_H__

#include "kanon/util/macro.h"
#include "kanon/string/string_view.h"
#include "kanon/util/optional.h"

#include <cstdint>

namespace kanon {

/**
 * strtoull() wrapper
 *
 * To robust, reuse the libc routines
 */
KANON_CORE_API optional<uint64_t> StringViewToU64(StringView const &sv,
                                                  int base = 10) KANON_NOEXCEPT;

/**
 * strtoul() wrapper
 */
KANON_CORE_API optional<uint32_t> StringViewToU32(StringView const &sv,
                                                  int base = 10) KANON_NOEXCEPT;

/**
 * strtoll() wrapper
 */
KANON_CORE_API optional<int64_t> StringViewTo64(StringView const &sv,
                                                int base = 10) KANON_NOEXCEPT;

/**
 * strtol() wrapper
 */
KANON_CORE_API optional<int32_t> StringViewTo32(StringView const &sv,
                                                int base = 10) KANON_NOEXCEPT;

KANON_INLINE optional<int16_t> StringViewTo16(StringView const &sv,
                                              int base = 10) KANON_NOEXCEPT
{
  return StringViewTo32(sv, base);
}

KANON_INLINE optional<uint16_t> StringViewToU16(StringView const &sv,
                                                int base = 10) KANON_NOEXCEPT
{
  return StringViewToU32(sv, base);
}

KANON_INLINE optional<int8_t> StringViewTo8(StringView const &sv,
                                            int base = 10) KANON_NOEXCEPT
{
  return StringViewTo32(sv, base);
}

KANON_INLINE optional<uint8_t> StringViewToU8(StringView const &sv,
                                              int base = 10) KANON_NOEXCEPT
{
  return StringViewToU32(sv, base);
}

} // namespace kanon

#endif // KANON_STRING_VIEW_UTIL_H__
