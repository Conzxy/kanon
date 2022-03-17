#include "kanon/process/process_info.h"

using namespace kanon::process;

int main() {
  ::puts(PidString().data());
  
  ::puts(Hostname().data());
  // check pid by process monitor tool such as glances or top
  while (1){}
}
