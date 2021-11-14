#include "kanon/net/EventLoop.h"
#include "kanon/thread/Thread.h"
#include "kanon/net/Channel.h"


#include <sys/timerfd.h>

using namespace kanon;

int main() {
  EventLoop loop{};
  
  auto timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  
  struct itimerspec timerspec;
  struct timespec spec;
  spec.tv_sec = 1;
  spec.tv_nsec = 0;

  timerspec.it_value = spec;
  timerspec.it_interval = spec;

  ::timerfd_settime(timerfd, 0, &timerspec, NULL);

  Channel timer_channel(&loop, timerfd);

  timer_channel.enableReading();
  timer_channel.set_read_callback([](){
      LOG_INFO << "timer out";
  });

  loop.loop();
}
