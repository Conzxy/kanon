#include "kanon/log/AsyncLog.h"
#include "kanon/log/Logger.h"
#include "kanon/thread/ThreadPool.h"

using namespace kanon;

#define NANOSECOND_PER_SECOND 1000000000

AsyncLog* g_asyncLog = nullptr;

void frontThreadFunc() {
	for (int i = 0; i != 100000; ++i)
		LOG_INFO << "Async Test";
}

int main(int , char** argv) {
	AsyncLog asyncLog{ ::basename(argv[0]), 20000 };
	
	Logger::setFlushCallback([&asyncLog]() {
			asyncLog.flush();
	});

	Logger::setOutputCallback([&asyncLog](char const* data, size_t len) {
			asyncLog.append(data, len);
	});

	asyncLog.start();

	ThreadPool pool{};
	pool.setMaxQueueSize(10);
	pool.start(200);
	 
	for (int i = 0; i < 20; ++i)
		pool.run(&frontThreadFunc);

	struct timespec sleepTime;
	BZERO(&sleepTime, sizeof sleepTime);
	sleepTime.tv_sec = 100000;

	::nanosleep(&sleepTime, NULL);
}
