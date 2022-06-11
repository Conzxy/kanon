#ifndef KANON_NET_ENDIAN_API_H
#define KANON_NET_ENDIAN_API_H

#include <stdint.h>
#include <endian.h>

#include "kanon/util/macro.h"

namespace kanon {

namespace sock {

// network byte order conversion function

/**
 * To network byte order
 */
inline uint8_t ToNetworkByteOrder8(uint8_t host8) noexcept
{ return host8; }

inline uint16_t ToNetworkByteOrder16(uint16_t host16) noexcept
{ return ::htobe16(host16); }

inline uint32_t ToNetworkByteOrder32(uint32_t host32) noexcept
{ return ::htobe32(host32); }

inline uint64_t ToNetworkByteOrder64(uint64_t host64) noexcept
{ return ::htobe64(host64); }

/**
 * To host byte order
 */
inline uint8_t ToHostByteOrder8(uint8_t net8) noexcept
{ return net8; }

inline uint16_t ToHostByteOrder16(uint16_t net16) noexcept
{ return ::be16toh(net16); }

inline uint32_t ToHostByteOrder32(uint32_t net32) noexcept
{ return ::be32toh(net32); }

inline uint64_t ToHostByteOrder64(uint64_t net64) noexcept
{ return ::be64toh(net64); }

} // namespace sock
} // namespace kanon

#endif // KANON_NET_ENDIAN_API_H