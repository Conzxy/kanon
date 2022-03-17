#include "kanon/string/fixed_buffer.h"
#include "kanon/string/stream_common.h"

namespace kanon {
namespace detail {

unsigned ptrToHexStr(char* buf, uintptr_t p) {
  static char const hex_digits[] = "0123456789ABCDEF";
  static_assert(sizeof hex_digits == 17, "hex_digits size should be 17");

  auto end = buf;
  auto count = 0;

  do {
    auto r = p % 16;
    p = p / 16;
    *(end++) = hex_digits[r];
    ++count;
  } while (p != 0);

  if (count < 16) {
    int rest = 16 - count;

    for (; rest > 0; --rest)
      *(end++) = '0';
  }

  std::reverse(buf, end);
  *end = 0;

  return end - buf;
}

template class FixedBuffer<kSmallStreamSize>;
template class FixedBuffer<kLargeStreamSize>;

} // namespace detail
} // namespace kanon