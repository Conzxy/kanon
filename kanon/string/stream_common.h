#ifndef KANON_STRING_STREAM_COMMON_H
#define KANON_STRING_STREAM_COMMON_H

#include <stdint.h>

namespace kanon {

constexpr int kSmallStreamSize = 4096;
constexpr int64_t kLargeStreamSize = 4096 << 10;
constexpr int kCastStreamSize = 64;

} // namespace kanon

#endif // KANON_STRING_STREAM_COMMON_H