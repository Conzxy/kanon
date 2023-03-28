#include "kanon/util/time.h"
#include "kanon/util/time_stamp.h"

#include <iostream>

using namespace kanon;

int main()
{
  TimeStamp cur_time = TimeStamp::Now();
  std::cout << cur_time.ToFormattedString() << "\n";
  std::cout << cur_time.seconds();
}
