#include "kanon/net/timer/itimer_queue_platform.h"

using namespace kanon;

ITimerQueuePlatform::ITimerQueuePlatform(EventLoop *loop)
  : loop_(loop)
{
}

ITimerQueuePlatform::~ITimerQueuePlatform() noexcept {}