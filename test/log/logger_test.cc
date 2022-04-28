#include "kanon/log/logger.h"

using namespace kanon;

#define N 100

int main(int argc, char* argv[]) {
  int cnt = 0;
  
  for (int i = 0; i != N; ++i) {
    LOG_INFO << "abcd" << "efgh" << ++cnt;
  }
  
  const std::string fmt = "a % b % c % d %";
  for (int i = 0; i != N; ++i) {
    FMT_LOG_INFO(fmt.data(), 1, 2, 3, ++cnt);
  }

}
