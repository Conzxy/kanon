#ifndef KANON_NET_ENDIAN_API_H
#define KANON_NET_ENDIAN_API_H

#include "kanon/util/macro.h"

#include <stdint.h>
#include <endian.h>

namespace kanon {

namespace sock {

// network byte order conversion function
inline uint16_t toNetworkByteOrder16(uint16_t host16) KANON_NOEXCEPT
{ return ::htobe16(host16); }

inline uint32_t toNetworkByteOrder32(uint32_t host32) KANON_NOEXCEPT
{ return ::htobe32(host32); }

inline uint64_t toNetworkByteOrder64(uint64_t host64) KANON_NOEXCEPT
{ return ::htobe64(host64); }

inline uint16_t toHostByteOrder16(uint16_t net16) KANON_NOEXCEPT
{ return ::be16toh(net16); }

inline uint32_t toHostByteOrder32(uint32_t net32) KANON_NOEXCEPT
{ return ::be32toh(net32); }

inline uint32_t toHostByteOrder64(uint64_t net64) KANON_NOEXCEPT
{ return ::be64toh(net64); }

} // namespace sock
} // namespace kanon

#endif
