#include "../http_request_parser.h"
#include "kanon/log/logger.h"
#include "kanon/string/string_view.h"

#include <gtest/gtest.h>

using namespace kanon;
using namespace http;

TEST(parser, request_line) {

  HttpRequestParser parser;
  auto ret = parser.ProcessRequestLine("GET http://hw:80/1.txt HTTP/1.1\r\n");

  EXPECT_EQ(ret, ParseResult::kComplete);  
  EXPECT_EQ(parser.GetMethod(), HttpMethod::kGet);
  EXPECT_EQ(parser.GetHttpVersion(), HttpVersion::kHttp11);

  LOG_INFO << "URL: " << parser.GetUrl();
  parser.Reset();

  LOG_INFO << "Method is not upper letter only";
  ret = parser.ProcessRequestLine("get http://hw:80/1.txt HTTP/1.1\r\n");

  EXPECT_EQ(ret, ParseResult::kBad);
  LOG_INFO << "Error Message: " << parser.GetErrorString();

  parser.Reset();
  ret = parser.ProcessRequestLine("GET http://hw:80/cgi/test?k=v HTTP/1.1\r\n");

  EXPECT_EQ(ret, ParseResult::kComplete);
  LOG_INFO << parser.GetUrl();

  EXPECT_EQ(parser.IsStatic(), false);
}

TEST(parser, parse) {
  HttpRequestParser parser;
  Buffer buffer;

  buffer.Append(MakeStringView(
    "GET http://host/ HTTP/1.1\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 88\r\n"
    "\r\n"
    "xxx"
    )
  );

  auto result = parser.Parse(buffer);
  EXPECT_EQ(result, ParseResult::kComplete);

  EXPECT_EQ(parser.GetCacheContentLength(), 88);
}

int main() {
  testing::InitGoogleTest();

  return RUN_ALL_TESTS();
}
