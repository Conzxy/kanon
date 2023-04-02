#define FORWARD_LIST_DEBUG
#include "kanon/algo/forward_list.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <stdlib.h>

#define N 100000

using namespace zstl;

template <typename T>
using FL = ForwardList<T>;

template <typename List>
bool equal_list(List const &list,
                std::initializer_list<typename List::ValueType> il)
{
  return std::equal(list.begin(), list.end(), il.begin());
}

// No need to check push_fron(Node*)
// since push_front(ValueType const&) has used
TEST(forward_list, push_front)
{
  ForwardList<int> list{};
  EXPECT_TRUE(list.empty());

  list.push_front(1);
  list.push_front(2);

  EXPECT_TRUE(equal_list(list, {2, 1}));
  EXPECT_EQ(list.size(), 2);
  EXPECT_EQ(list.front(), 2);
  EXPECT_EQ(list.back(), 1);
  EXPECT_EQ(*list.cbefore_end(), 1);
  EXPECT_EQ(list.end().extract(), nullptr);
}

TEST(forward_list, push_back)
{
  ForwardList<int> list;
  EXPECT_TRUE(list.empty());

  list.push_back(1);
  list.push_back(2);
  list.push_front(3);

  EXPECT_TRUE(!list.empty());
  EXPECT_TRUE(equal_list(list, {3, 1, 2}));
  EXPECT_EQ(list.front(), 3);
  EXPECT_EQ(list.back(), 2);
  EXPECT_EQ(list.size(), 3);
}

TEST(forward_list, extract_front)
{
  ForwardList<int> list{1, 2, 3, 4, 5};

  auto node = list.extract_front();
  EXPECT_TRUE(equal_list(list, {2, 3, 4, 5}));

  *node = 100;
  list.push_back(node);

  EXPECT_TRUE(equal_list(list, {2, 3, 4, 5, 100}));
  EXPECT_EQ(list.front(), 2);
  EXPECT_EQ(list.size(), 5);
  EXPECT_EQ(*list.cbefore_end(), 100);
  EXPECT_EQ(list.back(), 100);

  auto beg = list.cbegin();
  for (int i = 2; i < 6; ++i) {
    EXPECT_EQ(i, *beg++);
  }
}

TEST(forward_list, pop_front)
{
  ForwardList<int> list{1, 2, 3, 4, 5};

  EXPECT_EQ(list.size(), 5);
  EXPECT_EQ(list.back(), 5);
  EXPECT_TRUE(!list.empty());

  list.pop_front();
  list.pop_front();

  EXPECT_EQ(list.size(), 3);
  EXPECT_EQ(list.front(), 3);
  EXPECT_EQ(list.back(), 5);
}

TEST(forward_list, insert_after)
{
  FL<int> l{3, 4, 5, 6, 7, 8};

  auto iter_6 = zstl::advance_iter(l.begin(), 3);
  l.insert_after(iter_6, 100);
  equal_list(l, {3, 4, 5, 6, 100, 7, 8});

  FL<int> l1{2, 3, 4, 5, 6};
  l1.insert_after(l1.begin(), 10, 1);

  EXPECT_EQ(l1.front(), 2);

  auto beg = l1.begin().next();
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(*beg++, 1);
  }

  for (int i = 3; i <= 6; ++i) {
    EXPECT_EQ(*beg++, i);
  }

  auto il = {11, 22, 33, 44, 55, 66};
  FL<int> l2 = {1, 2, 3};
  l2.insert_after(l2.begin().next(), il.begin(), il.end());

  auto beg2 = l2.begin();
  EXPECT_EQ(*beg2++, 1);
  EXPECT_EQ(*beg2++, 2);

  for (uint32_t i = 0; i < il.size(); ++i) {
    EXPECT_EQ(*beg2++, 11 * (i + 1));
  }

  EXPECT_EQ(*beg2++, 3);
  EXPECT_EQ(beg2.extract(), nullptr);
}

TEST(forward_list, erase_after)
{
  ForwardList<int> list{1, 2, 3, 4, 5};

  list.erase_after(list.cbefore_begin());
  EXPECT_TRUE(equal_list(list, {2, 3, 4, 5}));

  puts("erase_after position pass(1)");

  // Check erase the last element
  list.erase_after(zstl::advance_iter(list.cbegin(), 2));
  EXPECT_TRUE(equal_list(list, {2, 3, 4}));
  puts("erase_after postition(last element) pass(2)");

  list.erase_after(list.cbegin());
  EXPECT_TRUE(equal_list(list, {2, 4}));
  EXPECT_EQ(list.size(), 2);
  puts("erase_after position pass(3)");

  FL<int> l1{1, 2, 3, 4, 5, 6, 7};
  l1.erase_after(l1.begin(), zstl::advance_iter(l1.begin(), 4));
  EXPECT_EQ(l1.size(), 4);
  l1.print();
  EXPECT_TRUE(equal_list(l1, {1, 5, 6, 7}));

  puts("erase_after range pass(4)");
}

// No need to test clear() since destructor include
TEST(forward_list, empty)
{
  ForwardList<int> list{2, 3, 4};
  list.clear();
  EXPECT_TRUE(list.empty());

  ForwardList<int> list1{2, 3, 4};
  list1.erase_after(list1.cbefore_begin(), list1.cend());
  EXPECT_TRUE(list1.empty());
}

TEST(forward_list, search_before)
{
  FL<int> list{1, 2, 3, 4, 5};

  auto it1 = list.search_before(3);
  EXPECT_TRUE(it1 != list.end());
  EXPECT_EQ(*it1, 2);

  EXPECT_TRUE(list.search_before(7) == list.end());

  auto it2 = list.search_before(3, zstl::advance_iter(list.begin(), 2));

  EXPECT_TRUE(it2 == list.end());
}

TEST(forward_list, swap)
{
  std::vector<int> data1({1, 2, 3, 4, 5});
  std::vector<int> data2({6, 7, 8, 9, 0});
  FL<int> l1(data1.begin(), data1.end());
  FL<int> l2(data2.begin(), data2.end());

  l1.swap(l2);
  l1.insert_after(l1.before_begin(), 10);
  EXPECT_EQ(l1.front(), 10);
  l1.pop_front();

  l2.insert_after(l2.before_begin(), 11);
  EXPECT_EQ(l2.front(), 11);
  l2.pop_front();
  int i = 0;
  for (auto x : l1) {
    EXPECT_EQ(x, data2[i++]);
  }

  i = 0;
  for (auto x : l2) {
    EXPECT_EQ(x, data1[i++]);
  }
}

TEST(forward_list, merge)
{
  FL<int> l1{1, 3, 5, 7, 9};
  FL<int> l2{2, 4, 6, 8, 10};

  l1.merge(l2);

  EXPECT_TRUE(std::is_sorted(l1.begin(), l1.end()));
  EXPECT_TRUE(equal_list(l1, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
  EXPECT_TRUE(equal_list(l2, {}));
}

#define GET_SORTED_LIST(list__)                                                \
  FL<int> list__;                                                              \
  std::vector<int> data__##list__;                                             \
  data__##list__.reserve(N);                                                   \
  for (int i = 0; i < N; ++i) {                                                \
    data__##list__.push_back((int)rand());                                     \
  }                                                                            \
  std::sort(data__##list__.begin(), data__##list__.end());                     \
  list__.insert_after(list__.before_begin(), data__##list__.begin(),           \
                      data__##list__.end());

TEST(forward_list, merge_large)
{
  srand(time(NULL));

  GET_SORTED_LIST(l1);
  GET_SORTED_LIST(l2);

  l1.merge(l2);

  ASSERT_EQ(std::is_sorted(l1.begin(), l1.end()), true);
  ASSERT_TRUE(l2.empty());

  l2.insert_after(l2.before_begin(), 11);
  ASSERT_EQ(l2.front(), 11);
}

TEST(forward_list, splice_after)
{
  FL<int> l1{1, 2, 3};
  FL<int> l2{4, 5, 6, 7};

  l1.splice_after(l1.begin(), l2, l2.begin());
  EXPECT_TRUE(equal_list(l1, {1, 5, 2, 3}));
  EXPECT_TRUE(equal_list(l2, {4, 6, 7}));
  puts("splice_after iterator pass(1)");

  FL<int> l3{8, 9, 10};

  l2.splice_after(l2.begin(), l3);
  l2.print();
  EXPECT_TRUE(equal_list(l2, {4, 8, 9, 10, 6, 7}));
  EXPECT_TRUE(equal_list(l3, {}));
  puts("splice_after list pass(2)");

  l1.splice_after(l1.before_begin(), l2, l2.begin(),
                  zstl::advance_iter(l2.begin(), 2));
  EXPECT_TRUE(equal_list(l1, {8, 1, 5, 2, 3}));
  EXPECT_TRUE(equal_list(l2, {4, 9, 10, 6, 7}));
  puts("splice_after range pass(3)");
}

TEST(forward_list, unique)
{
  FL<int> l1{2, 3, 3, 42, 99, 99, 10, 22, 22};

  l1.unique();

  EXPECT_TRUE(equal_list(l1, {2, 3, 42, 99, 10, 22}));
}

TEST(forward_list, reverse)
{
  FL<int> l1{1, 2, 3, 4, 5, 6, 7, 8, 9};
  l1.reverse();

  EXPECT_EQ(l1.front(), 9);
  EXPECT_EQ(l1.back(), 1);
  EXPECT_TRUE(equal_list(l1, {9, 8, 7, 6, 5, 4, 3, 2, 1}));
}

TEST(forward_list, remove)
{
  FL<int> l1{1, 2, 3, 4, 2, 3};

  EXPECT_EQ(l1.remove(2), 2);
  EXPECT_TRUE(equal_list(l1, {1, 3, 4, 3}));

  EXPECT_EQ(l1.remove(10), 0);
  EXPECT_EQ(l1.remove(3), 2);
  EXPECT_TRUE(equal_list(l1, {1, 4}));
}

TEST(forward_list, copy_ctor)
{
  FL<int> l1{2, 3, 4, 5, 6, 7};

  FL<int> l2 = l1;

  EXPECT_TRUE(l1 == l2);
}

TEST(forward_list, copy_assign)
{
  FL<int> l1{1, 2, 3, 4, 5, 6};

  FL<int> l2{2, 3, 4};
  EXPECT_EQ(l2 = l1, l1);
  EXPECT_EQ(*l2.before_end(), 6);
  puts("copy_assign less pass(1)");

  FL<int> l3{1, 1, 1, 1, 1, 1, 1};
  EXPECT_EQ(l3 = l1, l1);
  EXPECT_EQ(*l3.before_end(), 6);
  puts("copy_assign larger pass(2)");

  FL<int> l4;
  EXPECT_EQ(l4 = l1, l1);
  EXPECT_EQ(*l4.before_end(), 6);
  puts("copy_assign empty pass(3)");
}

TEST(forward_list, sort_small)
{
  FL<int> lst;

  for (int i = 10; i >= 0; --i) {
    lst.push_back(i);
  }

  lst.sort();
  EXPECT_TRUE(std::is_sorted(lst.begin(), lst.end()));
}

TEST(forward_list, sort_big)
{
  FL<int> lst;

  for (int i = N; i >= 0; --i) {
    lst.push_back(i);
  }

  lst.sort();
  EXPECT_TRUE(std::is_sorted(lst.begin(), lst.end()));
}

TEST(forward_list, sort_random)
{
  FL<int> l2;

  std::random_device r;
  std::default_random_engine e(r());
  std::uniform_int_distribution<int> uniform_dist(0, N);
  std::generate_n(std::back_inserter(l2), N, [&]() {
    return uniform_dist(e);
  });
  EXPECT_EQ(l2.size(), N);
  l2.sort();
  EXPECT_EQ(l2.size(), N);

  EXPECT_TRUE(std::is_sorted(l2.begin(), l2.end()));
}

TEST(forward_list, sort1)
{
  FL<int> l3;
  std::random_device r;
  std::default_random_engine e(r());
  std::uniform_int_distribution<int> uniform_dist(0, N);
  std::generate_n(std::back_inserter(l3), N, [&]() {
    return uniform_dist(e);
  });

  EXPECT_EQ(l3.size(), N);
  l3.sort2();
  EXPECT_EQ(l3.size(), N);
  // l3.print();

  EXPECT_TRUE(std::is_sorted(l3.begin(), l3.end()));
}

TEST(forward_list, resize)
{
  FL<int> l1;

  l1.resize(10, 2);
  equal_list(l1, {2, 2, 2, 2, 2, 2, 2, 2, 2, 2});
  puts("resize empty <- (10, 2) pass");

  l1.resize(2, 1);
  equal_list(l1, {2, 2});
  puts("resize (10, 2) <- (2, 1) pass");

  l1.resize(2, 0);
  equal_list(l1, {2, 2});
  puts("resize (2, 2) <- (2, 0) pass");
}

int main()
{
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
