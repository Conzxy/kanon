/**
 * Emulate concurrent client request service
 */
#include "kanon/net/user_client.h"
#include "kanon/rpc/rpc_channel.h"

#include "kanon/thread/count_down_latch.h"
#include "pb/simple.pb.h"

#include <vector>

using namespace std;
using namespace kanon;
using namespace kanon::protobuf::rpc;

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

  void Connect()
  {
    cli_->Connect();
    latch_.Wait();
  }
  void Disconnect()
  {
    cli_->Disconnect();
  }

  SimpleService::Stub &GetSimpleStub()
  {
    return stub_;
  }

 private:
  TcpClientPtr cli_;
  RpcChannel chan_;
  SimpleService::Stub stub_;
  mutable CountDownLatch latch_;
};

int main()
{
  EventLoopThread loop_thread;
  auto loop = loop_thread.StartRun();
  vector<std::unique_ptr<SimpleClient>> clis;

  CountDownLatch latch(10);
  for (int i = 0; i < 10; ++i) {
    clis.emplace_back(new SimpleClient(loop, InetAddr("127.0.0.1:9998")));
    // Wait connect successfully
    clis[i]->Connect();
    auto &stub = clis[i]->GetSimpleStub();

    std::unique_ptr<SimpleRequest> req(new SimpleRequest);

    req->set_i(i);
    auto response = new SimpleResponse;
    auto cli = clis[i].get();
    stub.simple(nullptr, GetPointer(req), response,
                PROTOBUF::NewCallable([cli, response, &latch]() {
                  DeferDelete<SimpleResponse> defer_response(response);
                  LOG_INFO << "response: " << response->i();
                  latch.Countdown();
                  cli->Disconnect();
                }));
  }

  latch.Wait();

  PROTOBUF::ShutdownProtobufLibrary();
}
