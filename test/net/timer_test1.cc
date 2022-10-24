#include "kanon/net/event_loop.h"
#include "kanon/log/logger.h"
#include "kanon/thread/thread.h"

using namespace kanon;

EventLoop* g_loop;

int main() {
  EventLoop loop;
  g_loop = &loop;
  
  loop.RunAfter([]() {
    LOG_INFO << "RunAfter 1s";
  }, 1.0);
  
  loop.RunAt([]() {
    LOG_INFO << "RunAt current_time + 1s";
  }, TimeStamp::Now() + TimeStamp::Second(1));

  Thread thr([]() {
    EventLoop loop1;
    g_loop->RunEvery([]() {
        LOG_INFO << "other thread call g_loop::RunEvery()";
    }, 1);
      
  });

  thr.StartRun();
  thr.Join();

  g_loop->StartLoop();
}

