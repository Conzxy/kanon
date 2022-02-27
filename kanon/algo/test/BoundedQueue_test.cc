#include "kanon/algo/BoundedQueue.h"
#include "kanon/thread/thread.h"
#include "kanon/log/logger.h"

#include <gtest/gtest.h>
#include <queue>

using namespace kanon;

#define N 10000000

struct A {
  int a;
  char n;
};

// TEST(STLQueue, push) {
//   std::queue<Thread> q;

//   for (int i = 0; i != N; ++i) {
//     q.emplace([]() {});
//   }
// }

// TEST(MYQueue, push) {
//   BoundedQueue<Thread> q{ N };

//   for (int i = 0; i != N; ++i) {
//     q.push([]() {});
//   }
// }

// TEST(STLQueue, emplacePOD) {
//   std::queue<A> q;

//   for (int i = 0; i != N; ++i) {
//     q.emplace(A{1, 'c'});
//   }
// }

// TEST(MyQueue, pushPOD) {
//   BoundedQueue<A> q{ N };

//   for (int i = 0; i != N; ++i) {
//     q.push(A{1, 'c'});
//   }
// }

int main() {
  BoundedQueue<int> queue{ 100 };

  for (int i = 0; i != 100; ++i) {
    queue.push(i);
  }

  EXPECT_TRUE(queue.size() == 100);
  EXPECT_TRUE(queue.max_size() == 100);

  for (int i = 0; i != 100; ++i) {
    LOG_INFO << queue.pop();
  }

  auto& base = queue.base();

  EXPECT_EQ(base.readable(), 0);

  for (int i = 0; i < 150; ++i) 
    queue.push(i);

  LOG_INFO << "========================";
  
  for (int i = 0; i != 100; ++i) {
    LOG_INFO << queue.pop();
  }

  ::testing::InitGoogleTest();
  //return RUN_ALL_TESTS();

}
