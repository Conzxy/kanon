#include "kanon/net/connector.h"
#include "kanon/net/event_loop.h"

using namespace kanon;

EventLoop* g_loop;

int main() {
  EventLoop loop;
  g_loop = &loop;

  InetAddr servAddr{ "127.0.0.1", 9999 };
  Connector connector{ g_loop, servAddr };

  connector.StartRun();
  g_loop->StartLoop();
}