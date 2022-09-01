#include <iostream>
#include <string>

#include "kanon/net/user_client.h"
#include "kanon/util/ptr.h"

#include "kanon/thread/count_down_latch.h"
#include <kanon/rpc/rpc_channel.h>

#include "pb/echo.pb.h"

using namespace kanon;
using namespace kanon::protobuf::rpc;

class EchoClient : noncopyable {
 public:
  EchoClient(EventLoop *loop, InetAddr const &addr)
    : cli_(NewTcpClient(loop, addr))
    , latch_(1)
    , chan_()
    , stub_(&chan_)
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

  EchoService::Stub &GetStub()
  {
    return stub_;
  }

 private:
  TcpClientPtr cli_;
  mutable CountDownLatch latch_;
  RpcChannel chan_;
  EchoService::Stub stub_;
};

int main()
{
  EventLoopThread thr_loop;

  auto loop = thr_loop.StartRun();

  EchoClient cli(loop, InetAddr("127.0.0.1:9998"));
  cli.Connect();

  auto &stub = cli.GetStub();

  EchoArgs args;
  EchoReply reply;
  std::string msg;

  std::cout << "Type a message: ";
  while (std::cin >> msg) {
    args.set_msg(msg);
    stub.Echo(NULL, &args, &reply, PROTOBUF::NewCallable([&reply]() {
                LOG_INFO << "reply: " << reply.msg();
                std::cout << "Type a message: ";
                std::cout.flush();
              }));
  }

  cli.Disconnect();
}
