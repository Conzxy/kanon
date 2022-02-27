#include "log/append_file.h"

using namespace kanon;

int main() {
  AppendFile file("a.txt");

  file.Append("sssssssssssssssssssss", 10);
}
