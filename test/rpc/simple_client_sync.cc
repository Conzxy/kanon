/**
 * Sync demo
 */
#include "kanon/log/logger.h"
#include "kanon/net/event_loop_thread.h"
#include "kanon/rpc/rpc_channel.h"
#include "kanon/net/user_client.h"
#include "kanon/rpc/rpc_controller.h"

#include "pb/simple.pb.h"

using namespace kanon::protobuf::rpc;
using namespace kanon;

class SimpleClient;

void Done(SimpleClient *cli, SimpleResponse *response);

void Done2(TcpClient *cli, SimpleResponse *response) {
  kanon::DeferDelete<SimpleResponse> defer_response(response);

  LOG_INFO << "Response's i = " << response->i();
  cli->Disconnect();
}

class SimpleClient : noncopyable {
 public:
  SimpleClient(EventLoop *loop, InetAddr const &serv_addr)
    : cli_(NewTcpClient(loop, serv_addr, "SimpleClient"))
    , chan_()
    , stub_(&chan_)
  {
    cli_->SetConnectionCallback([this](TcpConnectionPtr const &conn) {
      if (conn->IsConnected()) {
        chan_.SetConnection(conn);
        auto request = SimpleRequest();
        request.set_i(1);
        auto response = new SimpleResponse;
        // stub_.simple(NULL, &request, response,
        //              NewCallback(&Done, this, response));
        RpcController *controller = new RpcController();
        controller->SetTimeout(500);
        stub_.simple(controller, &request, response, NewCallback(&Done2, cli_.get(), response));
        delete controller;
      }
    });
  }

  void Connect() { cli_->Connect(); }
  void Disconnect() { cli_->Disconnect(); }

  EventLoop *GetLoop() noexcept { return cli_->GetLoop(); }
 private:
  TcpClientPtr cli_;
  RpcChannel chan_;
  SimpleService::Stub stub_;
};

void Done(SimpleClient *cli, SimpleResponse *response)
{
  kanon::DeferDelete<SimpleResponse> defer_response(response);

  LOG_INFO << "Response's i = " << response->i();
  cli->Disconnect();
  cli->GetLoop()->Quit();
}

int main()
{
  kanon::EventLoop loop;
  SimpleClient client(&loop, InetAddr("127.0.0.1:9998"));
  client.Connect();
  loop.StartLoop();
}
