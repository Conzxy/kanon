#ifndef KANON_NET_EVENT_H__
#define KANON_NET_EVENT_H__

#include "kanon/util/platform_macro.h"

#ifdef KANON_ON_UNIX
#include "kanon/linux/net/event.h"
#elif defined(KANON_ON_WIN)
#include "kanon/win/net/event.h"
#endif

#include <functional>

namespace kanon {

using ReadEventCallback = std::function<void(TimeStamp)>;
using EventCallback = std::function<void()>;

namespace detail {
// since no need to maintain event table
// we can reuse Channel::index_ to indicate event status in kerner events table
enum EventStatus {
  kNew = -1,  // event is never added to events table
  kAdded = 1, // event has added
};

} // namespace detail
} // namespace kanon

#endif