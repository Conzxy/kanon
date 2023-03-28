#ifndef KANON_NET_TIMER_TIMERQUEUE_H
#define KANON_NET_TIMER_TIMERQUEUE_H

#include "kanon/util/platform_macro.h"

#ifdef KANON_ON_UNIX
#include "kanon/linux/net/timer/timer_queue.h"
#elif defined(KANON_ON_WIN)
#include "kanon/win/net/timer/timer_queue.h"
#endif

#endif // KANON_NET_TIMER_TIMERQUEUE_H
