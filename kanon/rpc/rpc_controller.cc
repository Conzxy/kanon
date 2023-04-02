#include "rpc_controller.h"

#include "kanon/rpc/logger.h"
#include "kanon/net/event_loop.h"

#include "kanon/rpc/rpc.pb.h"

using namespace kanon::protobuf::rpc;
using namespace kanon;

RpcController::RpcController()
  : deadline_((Deadline)-1)
{
}

RpcController::~RpcController() noexcept {}

void RpcController::Reset()
{
  deadline_ = (Deadline)-1;
}

bool RpcController::Failed() const
{
  return false;
}

void RpcController::StartCancel() {}

void RpcController::SetFailed(std::string const & /*reason*/) {}

std::string RpcController::ErrorText() const
{
  return "";
}

bool RpcController::IsCanceled() const
{
  if (deadline_ != (Deadline)-1) {
    uint64_t now_ms = TimeStamp::Now().GetMicrosecondsSinceEpoch() / 1000;
    LOG_DEBUG_KANON_PROTOBUF_RPC << "Now: " << now_ms;
    LOG_DEBUG_KANON_PROTOBUF_RPC << "deadline: " << deadline();
    if (now_ms > deadline()) return true;
  }

  return false;
}

void RpcController::NotifyOnCancel(::google::protobuf::Closure *cb)
{
  KANON_UNUSED(cb);
}
