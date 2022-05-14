/**
 * Emulate short running pragram
 * However, async client is needed by long running program always(I think)
 * So, this is just a demo
 */
#include "kanon/net/event_loop_thread.h"
#include "kanon/rpc/krpc_client.h"
#include "kanon/log/logger.h"

#include "simple.pb.h"

using namespace kanon::protobuf::rpc;

void Done(KRpcClient* cli, SimpleResponse* response)
{
  kanon::DeferDelete<SimpleResponse> defer_response(response);

  LOG_INFO << "Response's i = " << response->i();
  cli->Disconnect();
}

int main()
{
  kanon::EventLoopThread thread_loop;
  const auto loop = thread_loop.StartRun();

  auto client = kanon::make_unique<KRpcClient>(loop, InetAddr("127.0.0.1", 9999), "SimpleClient");

  client->SetConnectionCallback([&client](TcpConnectionPtr const& conn) {
    client->OnConnection(conn);
    if (!conn->IsConnected()) {
      client->GetLatch().Countdown();
    }
  });

  client->Connect();

  const auto stub = client->GetStub<SimpleService::Stub>();

  auto request = SimpleRequest();
  request.set_i(1);
  auto response = new SimpleResponse;
  stub->simple(NULL, &request, response, NewCallback(&Done, GetPointer(client), response));

  auto& latch = client->GetLatch();
  latch.Wait();
}