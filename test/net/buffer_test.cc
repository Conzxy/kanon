#include "kanon/net/buffer.h"

#include <gtest/gtest.h>

using namespace std;
using namespace kanon;

char g_buf[1 << 16];

TEST(BufferTest, findCRLF)
{
  Buffer buffer;
  buffer.Append("findCRLF\r\n");

  string tmp;
  EXPECT_TRUE(buffer.FindCrLf(tmp));

  EXPECT_TRUE(::strcmp(tmp.data(), "findCRLF") == 0);

  Buffer buffer2;
  buffer2.Append("findCRLF");
  EXPECT_FALSE(buffer2.FindCrLf(tmp));
}

TEST(BufferTest, prepend)
{
  Buffer buffer;
  uint32_t i = 1;

  buffer.Append("222222");
  buffer.Prepend32(i);
  buffer.Append("33");
  EXPECT_EQ(buffer.GetReadBegin32(), 1);
}

TEST(BufferTest, append)
{
  Buffer buffer;
  buffer.Append(g_buf, sizeof g_buf);
}

TEST(BufferTest, multi_append)
{
  Buffer buffer;

  for (int i = 0; i < 16; ++i) {
    buffer.Append(g_buf, sizeof(g_buf) / 16);
  }
}

int main()
{
  testing::InitGoogleTest();

  return RUN_ALL_TESTS();
}
