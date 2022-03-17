#include "kanon/log/logger.h"
#include "kanon/log/log_file.h"
using namespace kanon;

#define N 100

std::unique_ptr<LogFile<>> g_logFile;

void output(char const* data, size_t num) noexcept {
  g_logFile->Append(data, num);
}

void flush() noexcept {
  g_logFile->flush();  
}

int main(int argc, char* argv[]) {
  //g_logFile.reset(new LogFile<>(::basename(argv[0]), 2000));
  //Logger::SetOutputCallback(&output);
  //Logger::SetFlushCallback(&flush);

  int cnt = 0;
  
  for (int i = 0; i != N; ++i) {
    LOG_INFO << "abcd" << "efgh" << ++cnt;
  }
  
  const std::string fmt = "a % b % c % d %";
  for (int i = 0; i != N; ++i) {
    FMT_LOG_INFO(fmt.data(), 1, 2, 3, ++cnt);
  }

}
