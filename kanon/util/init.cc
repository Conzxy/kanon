#include "kanon/util/init.h"
#include "kanon/thread/current_thread.h"

void kanon::KanonCoreInitialize()
{
  kanon::CurrentThread::MainThreadInitialize();
}