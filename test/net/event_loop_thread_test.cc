#include "kanon/net/event_loop_thread.h"
#include "kanon/net/event_loop.h"
#include "kanon/log/logger.h"

using namespace kanon;

int main() {
  EventLoopThread ev_loop_thread;

  auto p_loop = ev_loop_thread.StartRun();

  if (p_loop) {
    p_loop->Quit();

    LOG_INFO << "Loop has quited";
  }
}
