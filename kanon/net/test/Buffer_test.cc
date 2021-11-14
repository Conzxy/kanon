#include "kanon/net/Buffer.h"

#include <gtest/gtest.h>

using namespace std;
using namespace kanon;

TEST(BufferTest, findCRLF) {
  Buffer buffer;
  buffer.append("findCRLF\r\n");

  string tmp; 
  EXPECT_TRUE(buffer.findCRLF(tmp));

  EXPECT_TRUE(::strcmp(tmp.data(), "findCRLF") == 0);
  
  Buffer buffer2;
  buffer2.append("findCRLF");
  EXPECT_FALSE(buffer2.findCRLF(tmp));
}

TEST(BufferTest, prepend) {
  Buffer buffer;
  uint32_t i = 1;

  buffer.append("222222");
  buffer.prepend32(i);
  buffer.append("33");
  EXPECT_EQ(buffer.peek32(), 1);
}

int main() {
    testing::InitGoogleTest();

    return RUN_ALL_TESTS();
}