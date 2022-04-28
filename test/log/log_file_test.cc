#include "kanon/log/log_file.h"

using namespace kanon;

#define N 10000000
#define GB 1 << 30

int main(int, char* argv[]) {
  char const* basename = ::basename(argv[0]);

  std::string prefix = "/root/.log/";
  prefix += basename;
  prefix += "/";

  LogFile<> lf{ basename, GB , prefix, UINT_MAX, 3};
  
  SetupLogFile(lf);
  
  for (int i = 0; i < N; ++i) {
    LOG_INFO << "LogFile_test";
  }
}
