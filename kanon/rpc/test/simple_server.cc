#include "kanon/rpc/krpc_server.h"
#include "kanon/net/user_server.h"

#include "kanon/util/macro.h"
#include "simple.pb.h"

using namespace kanon::protobuf::rpc;

class SimpleServiceImpl : public SimpleService {
public:
  void simple(
      google::protobuf::RpcController* ,
      SimpleRequest const* request,
      SimpleResponse* response,
      google::protobuf::Closure* done) override
  {
    assert(request->has_i());
    response->set_i(request->i() * 10);

    done->Run();
  }

};

int main()
{
  EventLoop loop{};
  InetAddr addr(9999);
  KRpcServer server(&loop, addr, "simple rpc server", true);

  SimpleServiceImpl simple_service{};
  server.AddServices(&simple_service);

  server.StartRun();
  loop.StartLoop();
}
