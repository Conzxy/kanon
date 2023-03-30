#ifndef KANON_WIN_CORE_UTIL_ENDIAN_API_H_
#define KANON_WIN_CORE_UTIL_ENDIAN_API_H_

#include <winsock2.h>

namespace kanon {
namespace sock {

// network byte order conversion function

/**
 * To network byte order
 */
KANON_INLINE uint8_t ToNetworkByteOrder8(uint8_t host8) KANON_NOEXCEPT
{
  return host8;
}

KANON_INLINE uint16_t ToNetworkByteOrder16(uint16_t host16) KANON_NOEXCEPT
{
  return ::htons(host16);
}

KANON_INLINE uint32_t ToNetworkByteOrder32(uint32_t host32) KANON_NOEXCEPT
{
  return ::htonl(host32);
}

KANON_INLINE uint64_t ToNetworkByteOrder64(uint64_t host64) KANON_NOEXCEPT
{
  return ::htonll(host64);
}

/**
 * To host byte order
 */
KANON_INLINE uint8_t ToHostByteOrder8(uint8_t net8) KANON_NOEXCEPT { return net8; }

KANON_INLINE uint16_t ToHostByteOrder16(uint16_t net16) KANON_NOEXCEPT
{
  return ::ntohs(net16);
}

KANON_INLINE uint32_t ToHostByteOrder32(uint32_t net32) KANON_NOEXCEPT
{
  return ::ntohl(net32);
}

KANON_INLINE uint64_t ToHostByteOrder64(uint64_t net64) KANON_NOEXCEPT
{
  return ::ntohll(net64);
}

} // namespace sock
} // namespace kanon

#endif