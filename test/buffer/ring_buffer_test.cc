#include "kanon/buffer/ring_buffer.h"

#include <gtest/gtest.h>

using namespace kanon;

static char hex[] = "0123456789abcdef";

TEST(ring_buffer, append) {
  RingBuffer buffer(20);

  char data[] = "ring-buffer";
  size_t len = sizeof data - 1;
  buffer.Append(data, len);

  EXPECT_EQ(buffer.GetReadableSize(), len);
  EXPECT_EQ(buffer.RetrieveAllAsString(), data);

  buffer.Append(data, len);
  EXPECT_EQ(buffer.GetReadableSize(), 22);
  std::cout << buffer.RetrieveAllAsString() << '\n';
  
  
  buffer.Append(data, len);

  EXPECT_EQ(buffer.GetReadableSize(), 32);
  EXPECT_EQ(buffer.IsFull(), true);

  std::cout << buffer.RetrieveAllAsString() << '\n';
  std::cout << "data: " << std::string(buffer.GetData(), buffer.GetMaxSize()) << "\n\n";

  char buf[55];
  len = sizeof buf - 1;

  for (size_t i = 0; i < len; ++i) {
    buf[i] = hex[i % 16];
  }

  buffer.Append(buf, 32);
  std::cout << "buf case: " << buffer.RetrieveAllAsString() << "\n";
  std::cout << "data: " << std::string(buffer.GetData(), buffer.GetMaxSize()) << "\n\n";

  buffer.Append(buf + 32, len - 32);
  std::cout << "buf case: " << buffer.RetrieveAllAsString() << "\n";
  std::cout << "data: " << std::string(buffer.GetData(), buffer.GetMaxSize()) << "\n\n";
}

int main()
{
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}