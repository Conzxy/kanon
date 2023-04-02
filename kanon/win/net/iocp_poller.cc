#include "kanon/net/poll/iocp_poller.h"

#include "kanon/win/net/completion_context.h"
#include <windows.h>
#include <ioapiset.h>

using namespace kanon;
using namespace kanon::detail;

IocpPoller::IocpPoller(EventLoop *loop)
  : PollerBase{loop}
  , entries_(16)
{
  completion_port_ =
      CreateIoCompletionPort(INVALID_HANDLE_VALUE, // overlapped socket
                             NULL,                 // existing iocp
                             NULL,                 // socket context
                             1                     // thread num
      );

  if (!completion_port_) {
    LOG_SYSFATAL << "Failed to create io completion port";
  }
  LOG_TRACE_KANON << "Completion port: " << this << " has constructed";
}

IocpPoller::~IocpPoller() KANON_NOEXCEPT { ::CloseHandle(completion_port_); }

TimeStamp IocpPoller::Poll(int ms, ChannelVec &active_channels)
{
  ULONG removed_entries_num = 0;
  bool success = false;
  if (last_completion_cnt_ > 1) {
    success = GetQueuedCompletionStatusEx(completion_port_, &entries_[0],
                                          entries_.size(), &removed_entries_num,
                                          ms, FALSE);
  } else {
    LOG_TRACE_KANON << "Use GetQueueCompletionStatus()";

    auto *entry = &entries_[0];
    removed_entries_num = 1;
    success = GetQueuedCompletionStatus(
        completion_port_, &entry->dwNumberOfBytesTransferred,
        &entry->lpCompletionKey, &entry->lpOverlapped, ms);
  }

  TimeStamp now{TimeStamp::Now()};
  if (!success) {
    auto err = GetLastError();
    switch (err) {
      case WAIT_TIMEOUT:
        LOG_TRACE_KANON << "GetQueueCompletionStatus() timeout";
        break;
      case WAIT_IO_COMPLETION:
        LOG_TRACE_KANON << "APC queued, don't handle";
        break;
      /* Call GetQueuedCompletionStatus() may get error code
         ERROR_CONNECTION_REFUSED when WSA_IO_PENDING is returned
         by ConnectEx() */
      case ERROR_CONNECTION_REFUSED: {
        auto ctx = (CompletionContext *)entries_[0].lpOverlapped;
        (*ctx->write_event_callback)();
      } break;
      default:
        LOG_SYSFATAL << "Faild to call GetQueuedCompletionStatusEx()";
    }
    return now;
  }

  last_completion_cnt_ = removed_entries_num;
  if (removed_entries_num > 0) {
    LOG_TRACE_KANON << "Complection packet num = " << removed_entries_num;
    for (size_t i = 0; i < removed_entries_num; ++i) {
      auto &entry = entries_[i];
      auto ch = (Channel *)entry.lpCompletionKey;
      LOG_DEBUG_KANON << "Channel ID = [" << ch << "]";
      LOG_DEBUG_KANON << "Transferred bytes = " << entry.dwNumberOfBytesTransferred;
      ch->transferred_bytes = entry.dwNumberOfBytesTransferred;
      ch->PushCompleteContext((CompletionContext *)entry.lpOverlapped);
      active_channels.push_back(ch);
    }
    if (removed_entries_num == entries_.size()) {
      entries_.resize(removed_entries_num << 1);
    }
  } else {
    LOG_TRACE_KANON << "No packet has completed";
  }
  return now;
}

void IocpPoller::UpdateChannel(Channel *ch)
{
  if (ch->GetIndex() == EventStatus::kNew) {
    LOG_TRACE_KANON << "Register channel: [" << ch << "] to completion port";
    LOG_TRACE_KANON << "Fd = " << ch->GetFd();
    ch->SetIndex(EventStatus::kAdded);
    if (!CreateIoCompletionPort((HANDLE)ch->GetFd(), completion_port_,
                                (DWORD_PTR)ch, 0))
    {
      LOG_SYSFATAL << "Failed to associate socket to completion port";
    }
  }
}

void IocpPoller::RemoveChannel(Channel *ch) {}
