#include "../thread.h"
#include <iostream>

using namespace zxy;

int main(int argc, char* argv[])
{
	int x;
	Thread thr([&x](){
		printf("ThreadName: %s\n", zxy::CurrentThread::t_name);						
		x = 2;	
		});

	thr.start();
	
	std::cout << x << '\n';
	thr.join();
	std::cout << x << std::endl;

}
