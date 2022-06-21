#include "krpc_server.h"

#include "kanon/net/connection/tcp_connection.h"
#include "kanon/util/any.h"

using namespace kanon::protobuf::rpc;

KRpcServer::KRpcServer(
    EventLoop* loop,
    InetAddr const& addr,
    StringArg name,
    bool reuseport)
  : TcpServer(loop, addr, name, reuseport)
{
  SetConnectionCallback(
    [this](TcpConnectionPtr const& conn)
    {
      if (conn->IsConnected()) {
        auto channel = new KRpcChannel(conn);
        channel->SetServices(services_);
        conn->SetContext(channel);
      }
      else {
        delete *AnyCast<KRpcChannel*>(conn->GetContext());
      }
    });
}

KRpcServer::~KRpcServer() = default;
