#include "rpc_server.h"

#include "kanon/net/connection/tcp_connection.h"
#include "kanon/util/any.h"

using namespace kanon::protobuf::rpc;

RpcServer::RpcServer(EventLoop *loop, InetAddr const &addr, StringArg name,
                       bool reuseport)
  : TcpServer(loop, addr, name, reuseport)
{
  SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      auto channel = new RpcChannel(conn);
      channel->SetServices(services_);
      conn->SetContext(*channel);
    } else {
      auto p = AnyCast<RpcChannel>(conn->GetContext());
      assert(p);
      delete p;
    }
  });
}

RpcServer::~RpcServer() = default;
