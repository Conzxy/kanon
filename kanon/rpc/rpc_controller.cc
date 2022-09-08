#include "rpc_controller.h"

#include "kanon/net/event_loop.h"

#include "kanon/rpc/rpc.pb.h"

using namespace kanon::protobuf::rpc;
using namespace kanon;

RpcController::RpcController()
  : timeout_((Timeout)-1)
{
}

RpcController::~RpcController() noexcept {}

void RpcController::SetTimeout(Timeout tm) { timeout_ = tm; }

void RpcController::Reset() {
  timeout_ = (Timeout)-1;
}

bool RpcController::Failed() const
{
  return false;
}

void RpcController::StartCancel() {}

void RpcController::SetFailed(std::string const & /*reason*/) {}

std::string RpcController::ErrorText() const { return ""; }

bool RpcController::IsCanceled() const
{
  if (timeout_ != (Timeout)-1) {
    uint64_t now = TimeStamp::Now().GetMicrosecondsSinceEpoch() / 1000;
    if (now > timeout()) return true;
  }

  return false;
}

void RpcController::NotifyOnCancel(::google::protobuf::Closure *cb) {}
