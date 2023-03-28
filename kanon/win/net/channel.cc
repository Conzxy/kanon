#include "kanon/win/net/channel.h"

#include "kanon/net/event_loop.h"
#include "kanon/win/net/completion_context.h"

using namespace kanon;

Channel::Channel(EventLoop *loop, FdType fd)
  : fd_{fd}
  , index_{-1}
  , events_{0}
#ifndef NDEBUG
  , events_handling_{false}
#endif
  , loop_{loop}
{
  LOG_TRACE_KANON << "Channel fd = " << fd_ << " created";
}

Channel::~Channel() noexcept
{
#ifndef NDEBUG
  // In the handling events phase, close_callback_ remove the connection(i.e.
  // channel), it is unsafe, we should remove it in next phase(calling functor
  // phase)
  KANON_ASSERT(
      !events_handling_,
      "Events are being handled when Channel is destoyed.\n"
      "Advice: You should call EventLoop::QueueToLoop() to delay remove");
#endif
}

void Channel::Update()
{
  // This must be called in a event loop
  // We just check the eventloop of poller
  // is same with this
  loop_->UpdateChannel(this);
}

void Channel::Remove() noexcept { loop_->RemoveChannel(this); }

void Channel::HandleEvents(TimeStamp receive_time)
{
#ifndef NDEBUG
  events_handling_ = true;
#endif

  LOG_TRACE_KANON << "Event Receive Time: "
                  << receive_time.ToFormattedString(true);

  for (auto completion_context : completion_contexts_) {
    if (!completion_context) continue;
    auto revents = completion_context->event;
    LOG_TRACE_KANON << "fd = " << fd_ << ", Event = " << Ev2String(revents);

    if (revents & Event::ReadEvent) {
      if (transferred_bytes == 0 && *completion_context->close_event_callback) {
        (*completion_context->close_event_callback)();
      } else {
        if (*completion_context->read_event_callback)
          (*completion_context->read_event_callback)(receive_time);
      }
    }
    if ((revents & Event::WriteEvent) &&
        *completion_context->write_event_callback)
    {
      (*completion_context->write_event_callback)();
    }

    delete completion_context;
  }
  completion_contexts_.clear();
  transferred_bytes = -1;
#ifndef NDEBUG
  events_handling_ = false;
#endif
}

std::string Channel::Ev2String(uint32_t ev)
{
  std::string ret;
  ret.reserve(32);
  ret = "{";
  if (ev & Event::ReadEvent) {
    ret += " Read";
  }
  if (ev & Event::WriteEvent) {
    ret += " Write";
  }
  ret += " }";
  return ret;
}

void Channel::RegisterCompletionContext(CompletionContext *ctx)
{
  ctx->read_event_callback = &read_callback_;
  ctx->write_event_callback = &write_callback_;
  ctx->close_event_callback = &close_callback_;
  ctx->error_event_callback = &error_callback_;
}
