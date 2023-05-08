#include <gtest/gtest.h>

#include "kanon/string/type.h"
#include "kanon/string/fmt_stream.h"

#define TEST_STREAM(fmt, ...)                                                  \
  do {                                                                         \
    kanon::SmallFmtStream stream(fmt, __VA_ARGS__);                            \
    ::puts(kanon::SmallFmtStream().ToStringView().data());                     \
  } while (0)

#define TEST_STREAM2(fmt)                                                      \
  do {                                                                         \
    kanon::SmallFmtStream stream(fmt);                                         \
    ::puts(kanon::SmallFmtStream().ToStringView().data());                     \
  } while (0)

TEST(fmt_stream_test, int)
{
  int i = 1;
  TEST_STREAM("Connection [%] established", i);
  TEST_STREAM("a % b % c % d % ", 1, 2, 3, 4);
}

TEST(fmt_stream_test, double)
{
  double d = 2.0;
  TEST_STREAM("price: %", d);
}

TEST(fmt_stream_test, ptr)
{
  int i = 1;
  int *p = &i;

  TEST_STREAM("ptr: %", p);
  TEST_STREAM("ptr: %", nullptr);
}

TEST(fmt_stream_test, bool)
{
  bool b = true;

  TEST_STREAM("boolean: %", b);
  TEST_STREAM("boolean: %", false);
}

TEST(fmt_stream_test, str) { TEST_STREAM2("str: %%"); }

TEST(fmt_stream_test, assert)
{
  // Argument < %
  kanon::SmallFmtStream("str: % ");
  kanon::SmallFmtStream("str % %", 1);

  // Argument > %
  kanon::SmallFmtStream("str: % %", 1, 2, 3);
}

int main()
{
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
