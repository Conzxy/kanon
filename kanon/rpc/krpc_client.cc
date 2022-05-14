#include <functional>

#include "kanon/rpc/krpc_client.h"

namespace kanon {
namespace protobuf {
namespace rpc {

KRpcClient::KRpcClient(EventLoop* loop, InetAddr const& addr, std::string const& name)
  : TcpClient(loop, addr, name)
  , connected_cond_(connected_mutex_)
{
  SetConnectionCallback(std::bind(
    &KRpcClient::OnConnection, this, _1));
}

void KRpcClient::OnConnection(TcpConnectionPtr const& conn)
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


} // namespace rpc
} // namespace protobuf
} // namespace kanon