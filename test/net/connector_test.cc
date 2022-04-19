#include "kanon/net/connector.h"

#include <gtest/gtest.h>

#include "kanon/thread/thread.h"
#include "kanon/net/event_loop_thread.h"
#include "kanon/net/event_loop.h"
#include "kanon/log/logger.h"

using namespace kanon;

TEST(connector, start_run) {
  EventLoopThread loop_thread{};

  auto loop = loop_thread.StartRun();

  InetAddr servAddr{ "127.0.0.1", 9998 };
  auto connector = Connector::NewConnector(loop, servAddr);

  connector->StartRun();

  ::sleep(1);
}

TEST(connector, retry) {
  EventLoopThread loop_thread{};

  auto loop = loop_thread.StartRun();

  InetAddr servAddr{ "127.0.0.1", 9997 };
  auto connector = Connector::NewConnector(loop, servAddr);

  connector->StartRun();
  ::sleep(5);
}

TEST(connector, stop) {
  EventLoopThread loop_thread{};

  auto loop = loop_thread.StartRun();
  InetAddr serv_addr{ "127.0.0.1", 9996 };

  std::shared_ptr<Connector> connector = std::make_shared<Connector>(loop, serv_addr);

  connector->StartRun();

  Thread thr([connector]() {
    LOG_INFO << "Stop thread";
    connector->Stop();
    ::sleep(5);
  });

  thr.StartRun();
  thr.Join();
}

TEST(connector, restart) {
  EventLoopThread loop_thread{};

  auto loop = loop_thread.StartRun();
  InetAddr serv_addr{ "127.0.0.1", 9997 };

  std::shared_ptr<Connector> connector = std::make_shared<Connector>(loop, serv_addr);

  connector->StartRun();

  Thread thr([connector]() {
    sleep(5);
    connector->Restrat();
    sleep(3);
  });

  thr.StartRun();
  thr.Join();
}

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}