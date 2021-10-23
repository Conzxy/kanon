#include "kanon/net/EventLoop.h"
#include "kanon/thread/Thread.h"

using namespace kanon;

EventLoop* g_loop;

int main() {
	EventLoop loop;
	g_loop = &loop;
		
	Thread thr([]() {
		EventLoop loop1;
		g_loop->runEvery([]() {
				LOG_INFO << "other thread call g_loop::runEvery()";
		}, 1);
			
	});

	thr.start();
	thr.join();

	g_loop->loop();
}

