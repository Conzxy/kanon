#include <gtest/gtest.h>

#include "kanon/thread/thread.h"
#include "kanon/thread/atomic_counter.h"

using namespace kanon;

#define N 100000

AtomicCounter64 counter;

TEST(atomic_counter, integrity) {
  Thread thr([]() {
    for (int i = 0; i < N; ++i) {
      counter++;
    }
  });

  Thread thr2([]() {
    for (int i = 0; i < N; ++i) {
      counter++;
    }
  });

  thr.StartRun();
  thr2.StartRun();

  thr.Join();
  thr2.Join();

  EXPECT_EQ(counter.GetValue(), 2 * N);
}

int main() {
  ::testing::InitGoogleTest();

  return RUN_ALL_TESTS();
}