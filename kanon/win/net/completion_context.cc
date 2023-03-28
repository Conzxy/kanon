#include "kanon/win/net/completion_context.h"

#include <cstring>

void kanon::completion_context_init(CompletionContext *ctx)
{
  memset(&ctx->overlap, 0, sizeof(ctx->overlap));
  ctx->event = Event::NoneEvent;
  ctx->write_event_callback = nullptr;
  ctx->read_event_callback = nullptr;
  ctx->close_event_callback = nullptr;
  ctx->error_event_callback = nullptr;
}