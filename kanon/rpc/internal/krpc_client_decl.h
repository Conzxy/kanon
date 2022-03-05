#ifndef KANON_NET_KRPC_CLIENT_DECL_H
#define KANON_NET_KRPC_CLIENT_DECL_H

#include "kanon/net/user_client.h"
#include "kanon/thread/condition.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/thread/count_down_latch.h"
#include "kanon/rpc/krpc_channel.h"

namespace kanon {
namespace protobuf {
namespace rpc {

template<typename Stub>
class KRpcClient : public TcpClient {
public:
  using StubPtr = std::shared_ptr<Stub>;

  KRpcClient(EventLoop* loop, InetAddr const& addr, std::string const& name);

  StubPtr GetStub() const;
  
  CountDownLatch& GetLatch() const noexcept { return latch_; }
  void OnConnection(TcpConnectionPtr const& conn);
private:

  mutable CountDownLatch latch_;
  mutable MutexLock connected_mutex_;
  mutable Condition connected_cond_;
};

} // namespace rpc
} // namespace protobuf
} // namespace kanon

#endif