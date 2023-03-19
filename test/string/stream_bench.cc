#include <sstream>

#include <gtest/gtest.h>

#include "kanon/string/lexical_stream.h"
#include "kanon/string/fmt_stream.h"
#include "kanon/string/string_view.h"
#include "kanon/string/type.h"

#define NUM (10 * (1 << 18))

using namespace kanon;

static char const str[] = "0123456789abcdef";

static std::string strs = str;

// Release mode:
// lexical_stream > fmt_stream > ostringstream
TEST(bench, ostream) {
  std::ostringstream oss;
  oss.sync_with_stdio(false);

  for (unsigned i = 0; i < NUM; ++i) {
    oss << strs;
  }
}

TEST(bench, lexical_stream) {
  SmallLexicalStream stream;
  for (unsigned i = 0; i < NUM; ++i) {
    stream.reset();
    stream << str;
  }
}

TEST(bench, fmt_stream) {
  SmallFmtStream stream;
  for (unsigned i = 0; i < NUM; ++i) {
    stream.Reset();
    stream.Serialize(str);
  }
}

int main()
{
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
