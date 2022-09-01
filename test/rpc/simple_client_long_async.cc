/**
 * Emulate long running program
 * send rpc message to server asynchronously
 */
#include "kanon/log/logger.h"
#include "kanon/net/event_loop_thread.h"
#include "kanon/net/user_client.h"
#include "kanon/rpc/rpc_channel.h"
#include "kanon/thread/count_down_latch.h"

#include "pb/simple.pb.h"

using namespace kanon::protobuf::rpc;
using namespace kanon;

void Done(TcpClient *cli, SimpleResponse *response)
{
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
    , latch_(1)
  {
    cli_->SetConnectionCallback([this](TcpConnectionPtr const &conn) {
      if (conn->IsConnected()) {
        chan_.SetConnection(conn);
        latch_.Countdown();
      }
    });
  }

  void Simple(SimpleRequest const &request, SimpleResponse *response)
  {
    stub_.simple(NULL, &request, response,
                 NewCallback(&Done, cli_.get(), response));
  }

  void Connect()
  {
    cli_->Connect();
    latch_.Wait();
  }
  void Disconnect()
  {
    cli_->Disconnect();
  }

 private:
  TcpClientPtr cli_;
  RpcChannel chan_;
  SimpleService::Stub stub_;
  mutable CountDownLatch latch_;
};

int main()
{
  kanon::EventLoopThread thread_loop;
  const auto loop = thread_loop.StartRun();

  SimpleClient client(loop, InetAddr("127.0.0.1", 9998));

  client.Connect();

  auto request = SimpleRequest();
  request.set_i(1);
  auto response = new SimpleResponse;
  client.Simple(request, response);

  // Emulate a loop(e.g. Qt event loop)
  while (1)
    ;
}
