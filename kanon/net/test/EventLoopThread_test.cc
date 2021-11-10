#include "kanon/net/EventLoopThread.h"
#include "kanon/net/EventLoop.h"

using namespace kanon;

int main() {
  EventLoopThread ev_loop_thread;

  auto p_loop = ev_loop_thread.start();

  if (p_loop) {
    p_loop->quit();

    LOG_INFO << "Loop has quited";
  }
}
