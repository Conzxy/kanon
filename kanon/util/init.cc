#include "kanon/util/init.h"
#include "kanon/log/logger.h"
#include "kanon/thread/current_thread.h"

void kanon::KanonCoreInitialize()
{
  kanon::CurrentThread::MainThreadInitialize();
  LOG_TRACE_KANON << "Kanon core resources initialization: OK";
}

void kanon::KanonCoreTeardown() KANON_NOEXCEPT
{
  LOG_TRACE_KANON << "Kanon core resources teardown: OK";
}