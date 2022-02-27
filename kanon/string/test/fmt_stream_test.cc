#include <gtest/gtest.h>

#include "kanon/string/type.h"
#include "kanon/string/fmt_stream.h"

TEST(fmt_stream_test, int) {
  int i = 1;
  kanon::SmallFmtStream stream(
    "Connection [%] established", i);

  ::puts(stream.ToStringView().data());

  ::puts(kanon::SmallFmtStream("a % b % c % d % ", 1, 2, 3, 4).ToStringView().data());
}

TEST(fmt_stream_test, double) {
  double d = 2.0;
  kanon::SmallFmtStream stream(
    "price: %", d);
  ::puts(stream.ToStringView().data());
}

TEST(fmt_stream_test, ptr) {
  int i = 1;
  int* p = &i;

  ::puts(kanon::SmallFmtStream("ptr: %", p).ToStringView().data());
  ::puts(kanon::SmallFmtStream("ptr: %", nullptr).ToStringView().data());
}

TEST(fmt_stream_test, bool) {
  bool b = true;

  ::puts(kanon::SmallFmtStream("boolean: %", b).ToStringView().data());
  ::puts(kanon::SmallFmtStream("boolean: %", false).ToStringView().data());
}

TEST(fmt_stream_test, str) {
  ::puts(kanon::SmallFmtStream("str: %%").ToStringView().data());
}

TEST(fmt_stream_test, assert) {
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