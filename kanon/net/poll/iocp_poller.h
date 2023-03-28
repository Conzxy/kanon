#ifndef KANON_WIN_NET_IOCP_POLLER_H__
#define KANON_WIN_NET_IOCP_POLLER_H__

#include "kanon/net/poll/poller_base.h"

#include <vector>
#include <windows.h>
// #include <IoAPI.h>

namespace kanon {

class IocpPoller : public PollerBase {
 public:
  explicit IocpPoller(EventLoop *loop);

  virtual ~IocpPoller() noexcept;

  virtual TimeStamp Poll(int ms, ChannelVec &active_channels) override;
  virtual void UpdateChannel(Channel *ch) override;
  virtual void RemoveChannel(Channel *ch) override;

  HANDLE completion_port() noexcept { return completion_port_; }

 private:
  HANDLE completion_port_ = INVALID_HANDLE_VALUE;
  std::vector<OVERLAPPED_ENTRY> entries_;
};
} // namespace kanon

#endif