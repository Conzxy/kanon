#include "kanon/net/Buffer.h"

#include <gtest/gtest.h>

using namespace kanon;

TEST(BufferTest, findCRLF) {
    Buffer buffer;
    buffer.append("aaaaaaaaaaaaaa\r\n");
    auto sv = buffer.findCRLF();

    char buf[64];

    ::strncpy(buf, sv.data(), sv.size());
    EXPECT_STREQ(buf, "aaaaaaaaaaaaaa");
}

int main() {
    testing::InitGoogleTest();

    return RUN_ALL_TESTS();
}