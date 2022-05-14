#ifndef KANON_NET_KRPC_CLIENT_H
#define KANON_NET_KRPC_CLIENT_H

#include "kanon/net/user_client.h"
#include "kanon/thread/condition.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/thread/count_down_latch.h"
#include "kanon/rpc/krpc_channel.h"
#include "kanon/util/ptr.h"

namespace kanon {
namespace protobuf {
namespace rpc {

class KRpcClient : public TcpClient {
public:
  KRpcClient(EventLoop* loop, InetAddr const& addr, std::string const& name);

  template<typename Stub>
  std::shared_ptr<Stub> GetStub() const
  {
    while (!GetConnection() || !GetConnection()->IsConnected()) {
      connected_cond_.Wait();
    }

    const auto chan = *AnyCast<KRpcChannel*>(GetConnection()->GetContext());
    return std::make_shared<Stub>(chan);
  }

  CountDownLatch& GetLatch() const noexcept 
  { 
    if (latch_ == nullptr) {
      latch_= kanon::make_unique<CountDownLatch>(1);
    }
    return *latch_;
  }

  void OnConnection(TcpConnectionPtr const& conn);
private:

  mutable std::unique_ptr<CountDownLatch> latch_;
  mutable MutexLock connected_mutex_;
  mutable Condition connected_cond_;
};

} // namespace rpc
} // namespace protobuf
} // namespace kanon

#endif