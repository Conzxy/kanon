#include "kanon/net/EventLoopThread.h"
#include "kanon/net/EventLoop.h"

using namespace kanon;

int main() {
  //EventLoopThread loop_thr;

  //auto loop = loop_thr.start();
  
  EventLoop* loop = new EventLoop{};
  
  loop->runEvery([]() {
      LOG_INFO << "runEvery 1 seconds";
  }, 1);
  
  loop->runEvery([]() {
      LOG_INFO << "runEvery 1.5 seconds";
  }, 1.5);
  
  TimerId timer_id2;
  // can not handle self-cancel
  timer_id2 = loop->runEvery([&loop, &timer_id2]() {
    LOG_INFO << "self-cancel";
    loop->cancelTimer(timer_id2);          
  }, 1);
  
  loop->loop();

}
