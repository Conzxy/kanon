#include "kanon/string/lexical_cast.h"
#include <gtest/gtest.h>

using namespace kanon;

TEST(LexicalCastTest, Int) {
  char buf[32];
  strcpy(buf, lexical_cast<char const*>(322222222));
  EXPECT_EQ(0, strcmp(buf, "322222222"));
}

TEST(LexicalCastTest, Str2Long) {
  auto opt_long = lexical_cast<long>(MakeStringView("11111"));
  
  EXPECT_TRUE(opt_long.has_value());
  EXPECT_EQ(*opt_long, 11111);
  
  opt_long = lexical_cast<long>(MakeStringView("12345"));

  EXPECT_TRUE(opt_long);
  EXPECT_EQ(*opt_long, 12345);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
