#include "kanon/util/Any.h"

#include <stdio.h>
#include <gtest/gtest.h>

using namespace kanon;

struct Copy {
  Copy() = default;

  Copy(Copy const&) {
    puts("Copy's copy ctor");
  }
};

TEST(AnyTest, ctor) {
  Any i{ 2 };
  EXPECT_EQ(AnyCast<int>(i), 2);

  try {
    KANON_UNUSED(AnyCast<double>(i));
  } catch (BadAnyCastException const& ex) {
    puts("AnyCast is safe");
    puts(ex.what());
  }

  Any d{ 1.0 };
  EXPECT_EQ(AnyCast<double>(d), 1.0);

  Any ic{ i };
  EXPECT_EQ(AnyCast<int>(ic), AnyCast<int>(i));
  
  Any dc{ d };
  EXPECT_EQ(AnyCast<double>(dc), AnyCast<double>(d));

}

TEST(AnyTest, AnyCastWhenVIsNotRef) {
  Any x{ Copy{} };
  KANON_UNUSED(AnyCast<Copy>(x));
}

TEST(AnyTest, unsafeAnyCast) {
  int i = 1;
  Any ai = i;

  auto pai = unsafeAnyCast<int>(&ai);
  assert(pai);
  EXPECT_EQ(*pai, 1);
  
  auto pi = new int{ 42 };
  Any ap = pi;
  ::printf("pi address: %p\n", pi);

  auto p = AnyCast<int*>(&ap);
  ::printf("*p address: %p\n", *p);

  assert(p);
  EXPECT_EQ(**p, 42);
  delete *p;
}

int main() {
  testing::InitGoogleTest();

  return RUN_ALL_TESTS();
}
