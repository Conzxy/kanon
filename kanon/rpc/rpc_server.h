#ifndef KANON_RPC_KRPC_SERVER_H_
#define KANON_RPC_KRPC_SERVER_H_

#include <google/protobuf/message.h>

#include "rpc_channel.h"
#include "kanon/net/tcp_server.h"

namespace kanon {
namespace protobuf {
namespace rpc {

class RpcServer : public TcpServer {
public:
  RpcServer(
    EventLoop* loop,
    InetAddr const& addr,
    StringArg name,
    bool reuseport = false);

  ~RpcServer();

  inline void AddServices(PROTOBUF::Service* service);
private:
  RpcChannel::ServiceMap services_;
};

void RpcServer::AddServices(PROTOBUF::Service* service)
{
  services_.emplace(service->GetDescriptor()->full_name(), service);
}

} // namespace rpc
} // namespace protobuf
} // namespace kanon

#endif // KANON_RPC_KRPC_SERVER_H_
