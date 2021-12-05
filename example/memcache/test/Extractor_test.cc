#include "../Extractor.h"

#include <gtest/gtest.h>

using namespace kanon;
using namespace memcache;

TEST(Extractor_test, test1) {
  //StringView s{ makeStringView("1 2") };
  StringView s{ "1 2222" };
  StringViewSpaceTokenizer tokenizer{ s.begin(), s.end() };
  
  Extractor<' '> extractor{ tokenizer.begin(), tokenizer.end() };

  auto x1 = extractor.extract<long>(); 
  auto x2 = extractor.extract<int>(); 

  EXPECT_TRUE(x1);
  EXPECT_TRUE(x2);

  EXPECT_EQ(*x1, 1);
  EXPECT_EQ(x2.value(), 2222);

  auto x3 = extractor.extract<long>();

  EXPECT_FALSE(x3);
}

int main() {
  testing::InitGoogleTest();

  return RUN_ALL_TESTS(); 
}
