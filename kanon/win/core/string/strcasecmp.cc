#include "kanon/string/string_util.h"

#include <cassert>

int kanon::StrCaseCompare(char const *s1, char const *s2)
{
  unsigned char c1 = 0;
  unsigned char c2 = 0;

  assert(s1 && s2);
  for (;;) {
    c1 = (unsigned char)tolower(*s1++);
    c2 = (unsigned char)tolower(*s2++);
    if (c1 != c2 || c1 == 0 || c2 == 0) break;
  }
  return c1 - c2;
}

int kanon::StrNCaseCompare(char const *s1, char const *s2, size_t n)
{
  unsigned char c1 = 0;
  unsigned char c2 = 0;

  assert(s1 && s2);
  for (; n > 0; --n) {
    c1 = (unsigned char)tolower(*s1++);
    c2 = (unsigned char)tolower(*s2++);
    if (c1 != c2 || c1 == 0 || c2 == 0) break;
  }
  return c1 - c2;
}