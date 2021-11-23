#include "../HttpRequestParser.h"

#include <gtest/gtest.h>

IMPORT_NAMESPACE(http);

TEST(RequestParser, requestLine) {
  HttpRequestParser parser;
  auto ret = parser.parseRequestLine("GET http://hw:80/1.txt?sss#ss");
  
  EXPECT_TRUE((int)ret == 0);
}

int main() {
  testing::InitGoogleTest();

  return RUN_ALL_TESTS();
}
