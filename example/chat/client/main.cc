#include "chat_client.h"

#include "kanon/log/logger.h"
#include "kanon/log/async_log.h"
#include "kanon/net/event_loop_thread.h"

using namespace kanon;
using namespace std;

int main(int argc, char *argv[])
{
  // SetupAsyncLog(::basename(argv[0]), 20000, "/root/.log/");
  // kanon::SetKanonLog(true);
  kanon::EnableAllLog(true);
  // kanon::Logger::SetLogLevel(Logger::KANON_LL_TRACE);
  KanonNetInitialize();
  EventLoopThread loop_thread{};
  InetAddr serv_addr{"47.99.92.230", 9999};

  auto client = std::make_shared<ChatClient>(loop_thread.StartRun(), serv_addr);

  client->Connect();
  client->Start();
  client->Disconnect();
  std::this_thread::sleep_for(1s);
}
