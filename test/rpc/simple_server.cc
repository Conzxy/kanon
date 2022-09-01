#include "kanon/rpc/rpc_server.h"
#include "kanon/net/user_server.h"

#include "kanon/util/macro.h"
#include "pb/simple.pb.h"

using namespace kanon::protobuf::rpc;

class SimpleServiceImpl : public SimpleService {
public:
  void simple(
      google::protobuf::RpcController* ,
      SimpleRequest const* request,
      SimpleResponse* response,
      google::protobuf::Closure* done) override
  {
    kanon::DeferDelete<SimpleRequest const> defer_request(request);

    assert(request->has_i());
    response->set_i(request->i() * 10);

    done->Run();
  }

};

int main(int argc, char* argv[])
{
  if (argc < 2) {
    LOG_ERROR << "Usage: " << argv[0] << " port";
    return 0;
  }

  EventLoop loop{};

  InetAddr addr(::atoi(argv[1]));
  RpcServer server(&loop, addr, "simple rpc server");

  SimpleServiceImpl simple_service{};
  server.AddServices(&simple_service);

  server.StartRun();
  loop.StartLoop();

  LOG_INFO << "Shutdown protobuf lib";
  PROTOBUF::ShutdownProtobufLibrary();
}
