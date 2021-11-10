#include "kanon/process/process_info.h"

using namespace kanon::process;

int main() {
  ::puts(pidString());
  
  ::puts(hostname());
  // check pid by process monitor tool such as glances or top
  while (1){}
}
