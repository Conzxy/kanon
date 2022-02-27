#define CIRCULAR_BUFFER_DEBUG
#include "kanon/algo/CircularBuffer.h"
#include <gtest/gtest.h>

using namespace kanon;

TEST(circular_buffer, size_check)
{
  algo_util::CircularBuffer<int> buf(100);
  
  EXPECT_EQ(buf.readable(), 0);
  EXPECT_EQ(buf.max_size(), 100);
  EXPECT_EQ(buf.writeable(), 100);

  for (int i = 0; i < 10; ++i) {
    buf.emplace(i);
  }

  EXPECT_EQ(buf.readable(), 10);
  EXPECT_EQ(buf.writeable(), 90);

  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(buf.pop_front(), i);
  }
  
  EXPECT_EQ(buf.readable(), 0);
  EXPECT_EQ(buf.writeable(), 100);

  for (int i = 0; i < 100; ++i) {
    buf.emplace(i);
  } 

  EXPECT_EQ(buf.readable(), 100);
  EXPECT_EQ(buf.writeable(), 0);
  
  buf.print_memory_layout();

  EXPECT_EQ(buf.readable(), 100);
  EXPECT_EQ(buf.writeable(), 0);

  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(buf.pop_front(), i);
  }
}

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
