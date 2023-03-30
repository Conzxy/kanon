#ifndef KANON_WIN_COMPLETION_CONTEXT_H__
#define KANON_WIN_COMPLETION_CONTEXT_H__

#include <winsock2.h>

#include <functional>
#include "kanon/win/net/event.h"
#include "kanon/util/time_stamp.h"

namespace kanon {

struct CompletionContext {
  OVERLAPPED overlap;
  Event event;
  std::function<void()> *write_event_callback;
  std::function<void()> *close_event_callback;
  std::function<void(TimeStamp)> *read_event_callback;
  std::function<void()> *error_event_callback;
};

KANON_NET_NO_API void completion_context_init(CompletionContext *ctx);

} // namespace kanon

#endif