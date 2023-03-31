#include "kanon/net/init.h"

#include "kanon/log/logger.h"

void kanon::KanonNetInitialize()
{
  LOG_TRACE_KANON << "Kanon net resources initialization: OK";
}

void kanon::KanonNetTeardown() KANON_NOEXCEPT
{
  LOG_TRACE_KANON << "Kanon net resources teardown: OK";
}
