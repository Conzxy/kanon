#include "log/AppendFile.h"

using namespace kanon;

int main() {
  AppendFile file("a.txt");

  file.append("sssssssssssssssssssss", 10);
}
