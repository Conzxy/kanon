#include <iostream>

#include "simple.pb.h"

#include "kanon/util/noncopyable.h"
#include "kanon/net/tcp_client.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/inet_addr.h"
#include "kanon/net/tcp_connection.h"
#include "kanon/rpc/krpc_channel.h"

using namespace ::google::protobuf;
using namespace kanon;
using namespace kanon::protobuf::rpc;

class SimpleClient : kanon::noncopyable {
public:
  SimpleClient(EventLoop* loop, InetAddr const& addr)
    : client_(loop, addr, "Simple Client")
    , channel_(kanon::make_unique<KRpcChannel>())
  {
    client_.SetConnectionCallback(
      [this](TcpConnectionPtr const& conn)
      {
        if (conn->IsConnected()) {
          channel_->SetConnection(conn);
          SimpleRequest request;
          request.set_i(1);
          
          SimpleResponse* response = new SimpleResponse();
          SimpleService::Stub stub(channel_.get());
          stub.simple(NULL, &request, response, 
            NewCallback(this, &SimpleClient::done_of_simple, response));
        }
        else {
          LOG_DEBUG << "loop quit";
          client_.GetLoop()->Quit();
        }
      }
    );
  }

  void Connect()
  {
    client_.Connect();
  }

  void done_of_simple(SimpleResponse* response)
  {
    kanon::DeferDelete<SimpleResponse> defer_response(response);

    LOG_INFO << response->i();
    client_.Disconnect();
  }

  ~SimpleClient() = default;
private:
  TcpClient client_;
  std::unique_ptr<protobuf::rpc::KRpcChannel> channel_;
};

int main()
{
  EventLoop loop{};

  InetAddr addr("127.0.0.1", 9998);
  SimpleClient client(&loop, addr);

  client.Connect();
  loop.StartLoop();
}