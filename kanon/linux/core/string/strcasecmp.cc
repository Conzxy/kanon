#include "kanon/string/string_util.h"

int kanon::StrCaseCompare(char const *s1, char const *s2)
{
  return ::strcasecmp(s1, s2);
}

int kanon::StrNCaseCompare(char const *s1, char const *s2, size_t n)
{
  return ::strncasecmp(s1, s2, n);
}