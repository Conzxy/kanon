#include <iostream>

#include "../cgi_parse.h"

using namespace cgi;

int main()
{
  auto map = ParseQueryString("?num1=8&num2=9");

  for (auto kv : map) {
    std::cout << kv.first << "," << kv.second << '\n';
  }
}