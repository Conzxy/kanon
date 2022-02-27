#include "kanon/net/event_loop_thread.h"
#include "kanon/net/event_loop.h"

using namespace kanon;

int main() {
  //EventLoopThread loop_thr;

  //auto loop = loop_thr.StartRun();
  
  EventLoop* loop = new EventLoop{};
  
  loop->RunEvery([]() {
      LOG_INFO << "runEvery 1 seconds";
  }, 1);
  
  loop->RunEvery([]() {
      LOG_INFO << "runEvery 1.5 seconds";
  }, 1.5);
  
  TimerId timer_id2;
  // can not handle self-cancel
  timer_id2 = loop->RunEvery([&loop, &timer_id2]() {
    LOG_INFO << "self-cancel";
    loop->CancelTimer(timer_id2);          
  }, 1);
  
  loop->StartLoop();

}
