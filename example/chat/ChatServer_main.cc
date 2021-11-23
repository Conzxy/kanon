#include "ChatServer.h"

using namespace kanon;

int main() {
  InetAddr listen_addr{ 9999 };

  ChatServer chat_server{ listen_addr };

  chat_server.start();
}
