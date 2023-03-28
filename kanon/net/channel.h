#ifndef KANON_CHANNEL_H
#define KANON_CHANNEL_H

#include "kanon/util/platform_macro.h"
#ifdef KANON_ON_UNIX
#include "kanon/linux/net/channel.h"
#elif defined(KANON_ON_WIN)
#include "kanon/win/net/channel.h"
#endif

#endif // KANON_CHANNEL_H
