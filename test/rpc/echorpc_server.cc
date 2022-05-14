#include "echo.pb.h"
#include "kanon/log/logger.h"
#include "kanon/net/tcp_server.h"
#include "kanon/util/ptr.h"

#include <google/protobuf/service.h>
#include <kanon/rpc/krpc_server.h>
#include <kanon/net/user_server.h>

using namespace kanon;
using namespace kanon::protobuf::rpc;

class EchoServiceImpl : public EchoService {
  public:
    void Echo(PROTOBUF::RpcController*,
        EchoArgs const* args,
        EchoReply* reply,
        PROTOBUF::Closure* done) override {
      DeferDelete<EchoArgs const> echo_args_defer(args); 

      reply->set_msg(args->msg());

      done->Run();
    }
};

class EchoServer {
public:
  EchoServer(EventLoop* loop, InetAddr const& addr) 
    : server_(loop, addr, "EchoServer", false) {

    server_.AddServices(new EchoServiceImpl());
  }
  
  void StartRun() {
    server_.StartRun();
  }

private:
  KRpcServer server_;
};

int main() {
  SetKanonLog(false);
  EventLoop loop;
  
  EchoServer server(&loop, InetAddr(9998));

  server.StartRun();
  loop.StartLoop();
}
