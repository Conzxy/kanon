#include "kanon/net/connector.h"

#include <gtest/gtest.h>

#include "kanon/thread/thread.h"
#include "kanon/net/event_loop_thread.h"
#include <chrono>
#include "kanon/net/event_loop.h"
#include "kanon/log/logger.h"

using namespace kanon;
using namespace std::chrono;

static uint16_t g_port = 9996;

TEST(connector, start_run)
{
  EventLoopThread loop_thread{};

  auto loop = loop_thread.StartRun();

  InetAddr servAddr{"127.0.0.1", g_port++};
  auto connector = Connector::NewConnector(loop, servAddr);

  connector->StartRun();

  std::this_thread::sleep_for(1s);
}

TEST(connector, retry)
{
  EventLoopThread loop_thread{};

  auto loop = loop_thread.StartRun();

  InetAddr servAddr{"127.0.0.1", g_port++};
  auto connector = Connector::NewConnector(loop, servAddr);

  connector->StartRun();
  std::this_thread::sleep_for(5s);
}

TEST(connector, stop)
{
  EventLoopThread loop_thread{};

  auto loop = loop_thread.StartRun();
  InetAddr serv_addr{"127.0.0.1", g_port++};

  // std::shared_ptr<Connector> connector = std::make_shared<Connector>(loop,
  // serv_addr);
  auto connector = Connector::NewConnector(loop, serv_addr);
  connector->StartRun();
  connector->Stop();
}

TEST(connector, restart)
{
  EventLoopThread loop_thread{};

  auto loop = loop_thread.StartRun();
  InetAddr serv_addr{"127.0.0.1", g_port++};

  // std::shared_ptr<Connector> connector = std::make_shared<Connector>(loop,
  // serv_addr);
  auto connector = Connector::NewConnector(loop, serv_addr);

  connector->Stop();
  connector->StartRun();
  std::this_thread::sleep_for(3s);

  connector->Restrat();
  std::this_thread::sleep_for(1s);
}

int main()
{
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
