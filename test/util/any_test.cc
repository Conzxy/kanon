#include "kanon/util/any.h"

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
  EXPECT_EQ(*AnyCast<int>(i), 2);

  try {
    KANON_UNUSED(AnyCast<double>(i));
  } catch (BadAnyCastException const& ex) {
    puts("AnyCast is safe");
    puts(ex.what());
  }

  Any d{ 1.0 };
  EXPECT_EQ(*AnyCast<double>(d), 1.0);

  Any ic{ i };
  EXPECT_EQ(*AnyCast<int>(ic), *AnyCast<int>(i));
  
  Any dc{ d };
  EXPECT_EQ(*AnyCast<double>(dc), *AnyCast<double>(d));

}

TEST(AnyTest, AnyCastWhenVIsNotRef) {
  Any x{ Copy{} };
  KANON_UNUSED(AnyCast<Copy>(x));
}

TEST(AnyTest, unsafeAnyCast) {
  int i = 1;
  Any ai = i;

  auto pai = UnsafeAnyCast<int>(ai);
  assert(pai);
  EXPECT_EQ(*pai, 1);
  
  auto pi = new int{ 42 };
  Any ap = pi;
  ::printf("pi address: %p\n", pi);

  auto p = AnyCast<int*>(ap);
  ::printf("*p address: %p\n", *p);

  assert(p);
  EXPECT_EQ(**p, 42);
  delete *p;
}

TEST(AnyTest, moveable_object) {
  // Any any(std::unique_ptr<int>(new int(1)));
}

int main() {
  Any a(new int(1));
  delete *UnsafeAnyCast<int*>(a);

  testing::InitGoogleTest();

  return RUN_ALL_TESTS();
}
