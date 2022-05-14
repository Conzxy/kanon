#ifndef KANON_RPC_KRPC_CLIENT_IMPL_H
#define KANON_RPC_KRPC_CLIENT_IMPL_H

#include <functional>

#ifndef KANON_RPC_KRPC_CLIENT_H
#include "kanon/rpc/krpc_client.h"
#endif

namespace kanon {
namespace protobuf {
namespace rpc {

template<typename S>
KRpcClient<S>::KRpcClient(EventLoop* loop, InetAddr const& addr, std::string const& name)
  : TcpClient(loop, addr, name)
  , connected_cond_(connected_mutex_)
{
  SetConnectionCallback(std::bind(
    &KRpcClient::OnConnection, this, _1));
}

template<typename S>
void KRpcClient<S>::OnConnection(TcpConnectionPtr const& conn)
{
  if (conn->IsConnected()) {
    auto channel = new KRpcChannel(conn);
    channel->SetConnection(conn);
    conn->SetContext(channel);
    MutexGuard guard(connected_mutex_);
    connected_cond_.Notify();
  }
  else {
    delete *AnyCast<KRpcChannel*>(conn->GetContext());
  }
}

template<typename S>
auto KRpcClient<S>::GetStub() const -> StubPtr
{
  while (!GetConnection() || !GetConnection()->IsConnected()) {
    connected_cond_.Wait();
  }

  const auto chan = *AnyCast<KRpcChannel*>(GetConnection()->GetContext());
  return std::make_shared<S>(chan);
}

} // namespace rpc
} // namespace protobuf
} // namespace kanon

#endif