#include "log/Logger.h"
#include "log/LogFile.h"
using namespace kanon;

#define N 100000

std::unique_ptr<LogFile<>> g_logFile;

void output(char const* data, size_t num) KANON_NOEXCEPT {
  g_logFile->append(data, num);
}

void flush() KANON_NOEXCEPT {
  g_logFile->flush();  
}

int main(int argc, char* argv[]) {
  g_logFile.reset(new LogFile<>(::basename(argv[0]), 2000));

  Logger::setOutputCallback(&output);
  Logger::setFlushCallback(&flush);

  int cnt = 0;
  
  for (int i = 0; i != N; ++i) {
    LOG_INFO << "abcd" << "efgh" << ++cnt;
  }
}
