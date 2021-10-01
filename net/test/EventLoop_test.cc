#include "../EventLoop.h"
#include "thread/thread.h"
#include "thread/current-thread.h"
#include <stdio.h>

using namespace kanon;


void threadFunc() {
	printf("threadFunc: %s\n", CurrentThread::t_tidString);
	EventLoop loop;
	loop.loop();

}

int main() {
	printf("main: %s\n", CurrentThread::t_tidString);
	EventLoop loop;

	Thread thr{&threadFunc};
	thr.start();
	thr.join();

}
