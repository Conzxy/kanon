#include "kanon/util/optional.h"
#include "kanon/string/string_view.h"

#include <gtest/gtest.h>

using namespace kanon;

TEST(optional_test, ctor) {
  optional<int> op1{ 1234 };
  optional<StringView> op2{ "sssss" };

  EXPECT_TRUE(op1.has_value());
  EXPECT_TRUE(op2.has_value());
  EXPECT_EQ(op1.value(), 1234);
  EXPECT_EQ(op2.value(), "sssss");

  optional<int> op3;

  EXPECT_FALSE(op3.has_value());

  optional<int> op4{ op1 };

  EXPECT_TRUE(op4);
  EXPECT_EQ(*op4, 1234);
}

TEST(optional_test, assignment) {
  optional<int> op1{ 1 };
  optional<int> op2{ 2 };

  op2 = op1;
  EXPECT_TRUE(op1.has_value());
  EXPECT_TRUE(op2.has_value()); 

  EXPECT_EQ(op2.value(), 1);
  
  optional<int> op3{ 1111111111 };
  op2 = op3;

  EXPECT_TRUE(op2);
  EXPECT_EQ(*op2, 1111111111);
}

TEST(optional_test, option_U) {
  optional<int> op1{ 234 };
  optional<long> op2{ op1 };

  EXPECT_TRUE(op1);
  EXPECT_EQ(*op1, 234);
}

int main() {
  testing::InitGoogleTest();

  return RUN_ALL_TESTS();
}
