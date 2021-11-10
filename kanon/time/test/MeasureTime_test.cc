#include "kanon/time/MeasureTime.h"

using namespace kanon;

void func() {
  puts("func OK");
}

int main() {
  MEASURE_TIME(func);
}
