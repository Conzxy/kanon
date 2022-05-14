#include "kanon/rpc/krpc_client.h"
#include "simple.pb.h"

#include <vector>

using namespace std;
using namespace kanon;
// using namespace kanon::protobuf;

using SimpleClient = kanon::protobuf::rpc::KRpcClient<SimpleService::Stub>;

void Done(SimpleClient* cli, SimpleResponse* response) {
  DeferDelete<SimpleResponse> defer_response(response);
  LOG_INFO << "response: " << response->i();

  cli->Disconnect();
}

bool can_exit = false;

void Handler(int signo) {
  can_exit = true;
}

void RegisterSingalInt() {
  ::signal(SIGINT, Handler);
}

int main() {
  RegisterSingalInt();

  EventLoopThread loop_thread;
  auto loop = loop_thread.StartRun();
  vector<std::unique_ptr<SimpleClient>> clis;

  for (int i = 0; i < 10; ++i) {
    clis.emplace_back(new SimpleClient(loop, InetAddr("127.0.0.1:9998"), "SimpleClient"));
    clis[i]->Connect();
    auto stub = clis[i]->GetStub();

    std::unique_ptr<SimpleRequest> req(new SimpleRequest);

    req->set_i(i);
    auto response = new SimpleResponse;
    stub->simple(nullptr, GetPointer(req), response, PROTOBUF::NewCallback(&Done, GetPointer(clis[i]), response));
  }

  while (!can_exit) {}

  PROTOBUF::ShutdownProtobufLibrary();
}