/**
 * Emulate short running program
 * send rpc message to server asynchronously
 */

#include <future>

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

void ProtobufRpcCallbackWrapper(std::function<void()> *fn)
{
  (*fn)();
}

void ProtobufRpcCallbackWrapper2(std::function<void()> fn)
{
  fn();
}

PROTOBUF::Closure *NewRpcCallback(std::function<void()> *fn)
{
  return PROTOBUF::NewCallback(&ProtobufRpcCallbackWrapper, fn);
}

PROTOBUF::Closure *NewRpcCallback(std::function<void()> fn)
{
  return PROTOBUF::NewCallback(&ProtobufRpcCallbackWrapper2, std::move(fn));
}

struct TestCallable {
  TestCallable() = default;

  TestCallable(TestCallable const &)
  {
    printf("TestCallable copy ctor\n");
  }

  TestCallable(TestCallable &&)
  {
    printf("TestCallable move ctor\n");
  }

  void operator()() {}
};

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
      } else {
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
    latch_.Reset(1);
    cli_->Disconnect();
    latch_.Wait();
  }

  void SyncDisconnect()
  {
    cli_->Disconnect();
  }

  SimpleService::Stub &GetSimpleStub()
  {
    return stub_;
  }

  CountDownLatch &GetConnectLatch()
  {
    return latch_;
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
  auto &stub = client.GetSimpleStub();

  CountDownLatch latch(1);
#if 0
  stub.simple(NULL, &request, response, NewRpcCallback([&latch, response, &client]() {
    kanon::DeferDelete<SimpleResponse> defer_response(response);

    LOG_INFO << "Response's i = " << response->i();
    client.Disconnect();
    latch.Countdown();
  }));
#endif

  stub.simple(NULL, &request, response, NewCallable(TestCallable()));
  stub.simple(NULL, &request, response,
              NewCallable([&latch, response]() {
                kanon::DeferDelete<SimpleResponse> defer_response(response);

                LOG_INFO << "Response's i = " << response->i();
                latch.Countdown();
              }));

  latch.Wait();

  SimpleRequest request2;
  SimpleResponse response2;
  request2.set_i(10);

  std::promise<int> prm;
  stub.simple(NULL, &request2, &response2,
              NewCallable([&prm, &response2]() {
                // Test if the set_value() will block this thread
                sleep(1);
                prm.set_value(response2.i());
                LOG_INFO << "Disconnect successfully";
              }));

  LOG_INFO << "future: " << prm.get_future().get();
  client.Disconnect();
}
