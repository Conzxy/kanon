#include "kanon/net/Connector.h"
#include "kanon/net/EventLoop.h"

using namespace kanon;

EventLoop* g_loop;

int main() {
  EventLoop loop;
  g_loop = &loop;

  InetAddr servAddr{ "127.0.0.1", 9999 };
  Connector connector{ g_loop, servAddr };

  connector.start();
  g_loop->loop();
}