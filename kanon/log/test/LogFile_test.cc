#include "kanon/log/LogFile.h"
#include "kanon/log/Logger.h"

using namespace kanon;

#define N 1000

LogFile<>* g_logFile = nullptr;

int main(int, char* argv[]) {
  char const* basename = ::basename(argv[0]);

  std::string prefix = "/root/.log/";
  prefix += basename;
  prefix += "/";

  LogFile<> logFile{ basename, 20000 , prefix};
  
  g_logFile = &logFile;

  Logger::setOutputCallback([](char const* data, size_t num) {
    g_logFile->append(data, num);
  });

  Logger::setFlushCallback([]() {
    g_logFile->flush();
  });
  
  for (int i = 0; i < N; ++i) {
    LOG_INFO << "LogFile_test";
  }
}
