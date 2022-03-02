#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <string>

#include "kanon/util/mem.h"

#include "http_response.h"

#include "cgi_parse.h"

using namespace std;
using namespace cgi;
using namespace http;
using namespace kanon;

int main()
{
  auto method = ::getenv("REQUEST_METHOD");
  int num1 = 0, num2 = 0;

  if (method != NULL) {
    if (::strcmp(method, "GET") == 0) {
      auto query = ::getenv("QUERY_STRING");
      auto args_map = ParseQueryString(query);     


      auto iter = args_map.find("num1");
      if (iter != args_map.end()) {
        num1 = ::atoi(iter->second.c_str());
      }

      iter = args_map.find("num2");
      if (iter != args_map.end()) {
        num2 = ::atoi(iter->second.c_str());
      }
    }
    else if (::strcmp(method, "POST") == 0) {
      auto len = ::atoll(::getenv("CONTENT_LENGTH"));

      std::string content;
      content.reserve(len);
      ::fread(&content[0], 1, len, stdin);

      ::fwrite(content.data(), 1, len, stdout);
    }
  }

  HttpResponse response;

  char buf[4096];MemoryZero(buf);
  response.AddHeader("Connection", "close")
          .AddHeader("Content-type", "text/html")
          .AddBlackLine()
          .AddBody("<html>")
          .AddBody("<title>adder</title>")
          .AddBody("<body bgcolor=\"#ffffff\">")
          .AddBody("Welcome to add.com\r\n")
          .AddBody(buf, "<p>The answer is: %d + %d = %d</p>\r\n", num1, num2, num1+num2)
          .AddBody("<p>Thanks for visiting!</p>\r\n")
          .AddBody("</body>")
          .AddBody("</html>\r\n");

  auto buffer = response.GetBuffer().ToStringView();
  ::fwrite(buffer.begin(), 1, buffer.size(), stdout);
  ::fflush(stdout);
}