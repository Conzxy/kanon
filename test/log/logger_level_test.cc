#include "kanon/log/logger.h"

using namespace kanon;

#define FATAL_BIT kanon::Logger::KANON_LL_FATAL

int main()
{
  if ((kanon::Logger::KANON_LL_FATAL & kanon::Logger::KANON_LL_FATAL) &
      (kanon::Logger::KANON_LL_SYS_FATAL & kanon::Logger::KANON_LL_FATAL))
  {
    printf("These are Fatal levels\n");
  }

  if (!(kanon::Logger::KANON_LL_ERROR & FATAL_BIT)) {
    printf("There are not fatal level\n");
  }

  LOG_TRACE << "TRACE";
  LOG_DEBUG << "DEBUG";
  LOG_INFO << "INFO";
  LOG_WARN << "WARN";
  LOG_ERROR << "ERROR";
  // LOG_FATAL << "FATAL";
}
