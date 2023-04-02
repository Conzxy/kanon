#include "string_view_util.h"

#include <string.h> // memcpy
#include <assert.h> // assert
#include <stdlib.h> // strtoXXX()

using namespace kanon;

optional<uint64_t> kanon::StringViewToU64(StringView const &sv,
                                          int base) KANON_NOEXCEPT
{
  assert(sv.size() < 64);
  static char buf[64];
  memcpy(buf, sv.data(), sv.size());
  buf[sv.size()] = 0;
  char *end = NULL;
  uint64_t ret = strtoull(buf, &end, base);
  if (ret == 0 && end == buf) {
    return make_null_optional<uint64_t>();
  }
  return ret;
}

optional<uint32_t> kanon::StringViewToU32(StringView const &sv,
                                          int base) KANON_NOEXCEPT
{
  assert(sv.size() < 32);
  static char buf[32];
  memcpy(buf, sv.data(), sv.size());
  buf[sv.size()] = 0;
  char *end = NULL;
  uint32_t ret = (uint32_t)strtoul(buf, &end, base);
  if (ret == 0 && end == buf) {
    return make_null_optional<uint32_t>();
  }
  return ret;
}

optional<int32_t> kanon::StringViewTo32(StringView const &sv,
                                        int base) KANON_NOEXCEPT
{
  assert(sv.size() < 32);
  static char buf[32];
  memcpy(buf, sv.data(), sv.size());
  buf[sv.size()] = 0;
  char *end = NULL;
  int32_t ret = (int32_t)strtol(buf, &end, base);
  if (ret == 0 && end == buf) {
    return make_null_optional<int32_t>();
  }
  return ret;
}

optional<int64_t> kanon::StringViewTo64(StringView const &sv,
                                        int base) KANON_NOEXCEPT
{
  assert(sv.size() < 64);
  static char buf[64];
  memcpy(buf, sv.data(), sv.size());
  buf[sv.size()] = 0;
  char *end = NULL;
  int64_t ret = strtoll(buf, &end, base);
  if (ret == 0 && end == buf) {
    return make_null_optional<int64_t>();
  }
  return ret;
}

