#include "kanon/algo/reserved_array.h"

#include <gtest/gtest.h>

#include <string>

using namespace kanon::algo;
using namespace std;

TEST(reserved_array, construct) {
  puts("====== Defalut constructor + Grow =====");
  ReservedArray<int> arr2{};

  arr2.Grow(10);
  arr2[0] = 2;
  
  EXPECT_EQ(arr2[0], 2);
  
  ReservedArray<string> str_arr{};

  str_arr.Grow(10);

  str_arr[1] = "Conzxy";

  EXPECT_EQ(str_arr.GetSize(), 10);
  EXPECT_EQ(str_arr[1], "Conzxy");

  puts("===== size constructor =====");

  ReservedArray<int> arr(4);
  
  int i = 0;

  std::generate(arr.begin(), arr.end(), [i]() mutable {
      return i++;
  });

  for (size_t i = 0; i < arr.GetSize(); ++i) {
    EXPECT_EQ(arr[i], i);
  }
}

struct A {
  int i_;

  A(int i=0) noexcept 
    : i_(i)
  {}
  
  A(const A& ) = default;
  
  A& operator=(A const& ) = default;

  A(A&&) noexcept {
    puts("A move ctor");
  }

  A& operator=(A&&) noexcept {
    puts("A move assignment");
    return *this;
  }

  ~A() noexcept {}

  constexpr static bool can_reallocate = true;
};

struct B {
  B() {}
  B(B&&) noexcept {
    puts("B move ctor");
  }

  B& operator=(B&&) noexcept {
    puts("B move assignment");
    return *this;
  }

  ~B() noexcept {}
};

TEST(reserved_array, reallocate) {
  static_assert(!std::is_trivial<A>::value, "");

  static_assert(can_reallocate<A>::value, "");
  static_assert(!can_reallocate<B>::value, "");
  static_assert(can_reallocate<int>::value, "");
  
  puts("===== reserved_array reallocate test =====");

  ReservedArray<A> a{2};
  a.Grow(10);

  ReservedArray<B> b{2};
  b.Grow(10);
}

class ReservedArrayTest : public ::testing::Test {
 public:
  ReservedArrayTest()
    : arr_(100)
  { }

 protected:
  void SetUp() override {
    for (int i = 0; i < 100; ++i) {
      arr_[i] = i;
    }
  }

  ReservedArray<int> arr_;
};

TEST_F(ReservedArrayTest, copy_ctor) {
  ReservedArray<int> arr = arr_;

  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(arr[i], i);
  }
}

TEST_F(ReservedArrayTest, copy_assign) {
  ReservedArray<int> arr(20);
  
  puts("===== size(less than) =====");
  arr = arr_;

  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(i, arr[i]) << "size(less than)";
  }

  puts("===== size(equal) =====");

  ReservedArray<int> arr2(100);
  arr2 = arr_;

  for (int i = 0; i< 100; ++i) {
    EXPECT_EQ(arr2[i], i) << "size(equal)";
  }

  puts("===== size(greater than) =====");
  ReservedArray<int> arr3(120);

  arr3 = arr_;

  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(arr3[i], i) << "size(greater than)";
  }
}

class ReservedArrayTest2 : public ::testing::Test {
 public:
  ReservedArrayTest2()
    : arr_(100)
  {
  }

 protected:
  void SetUp() override {
    for (int i = 0; i < 100; ++i) {
      arr_[i].i_ = i;
    }

    for (int i = 0; i < 100; ++i) {
      EXPECT_EQ(arr_[i].i_, i);
    }
  }

  ReservedArray<A> arr_;
};

TEST_F(ReservedArrayTest2, copy_ctor) {
  ReservedArray<A> arr = arr_;

  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(arr[i].i_, i);
  }
}

TEST_F(ReservedArrayTest2, copy_assignment) {
  ReservedArray<A> arr(20);
  
  puts("===== size(less than) =====");
  arr = arr_;

  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(i, arr[i].i_) << "size(less than)";
  }

  puts("===== size(equal) =====");

  ReservedArray<A> arr2(100);
  arr2 = arr_;

  for (int i = 0; i< 100; ++i) {
    EXPECT_EQ(arr2[i].i_, i) << "size(equal)";
  }

  puts("===== size(greater than) =====");
  ReservedArray<A> arr3(120);
  arr3 = arr_;

  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(arr3[i].i_, i) << "size(greater than)";
  }
}
