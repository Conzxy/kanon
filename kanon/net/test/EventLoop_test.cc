#include "../EventLoop.h"
#include "kanon/thread/thread.h"
#include "kanon/thread/current-thread.h"
#include <stdio.h>


using namespace kanon;

std::unique_ptr<EventLoop> g_loop;

void threadFunc() {
	printf("threadFunc: %s\n", CurrentThread::t_tidString);
	g_loop->loop();

}

int main() {
	printf("main: %s\n", CurrentThread::t_tidString);
	
	g_loop.reset(new EventLoop{});
	Thread thr{&threadFunc};
	thr.start();
	thr.join();

}
