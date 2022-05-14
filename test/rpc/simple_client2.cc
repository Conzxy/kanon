/**
 * Emulate long running program
 * send rpc message to server
 */
#include "kanon/net/event_loop_thread.h"
#include "kanon/rpc/krpc_client.h"
#include "kanon/log/logger.h"

#include "simple.pb.h"

using namespace kanon::protobuf::rpc;

using SimpleRpcClient = KRpcClient<SimpleService::Stub>;

void Done(SimpleRpcClient* cli, SimpleResponse* response)
{
  kanon::DeferDelete<SimpleResponse> defer_response(response);

  LOG_INFO << "Response's i = " << response->i();
  cli->Disconnect();
}

int main()
{
  kanon::EventLoopThread thread_loop;
  const auto loop = thread_loop.StartRun();

  SimpleRpcClient client(loop, InetAddr("127.0.0.1", 9998), "SimpleClient");

  client.Connect();

  const auto stub = client.GetStub();

  auto request = SimpleRequest();
  request.set_i(1);
  auto response = new SimpleResponse;
  stub->simple(NULL, &request, response, NewCallback(&Done, &client, response));

  // Emulate a loop(e.g. Qt event loop)
  while (1);
}