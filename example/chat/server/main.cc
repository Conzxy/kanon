#include "chat_server.h"

using namespace kanon;

int main() {
  EventLoop loop;

  ChatServer chat_server(loop);

  chat_server.StartRun();
  loop.StartLoop();
}
