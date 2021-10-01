#include "../errlog.h"
#include <gtest/gtest.h>

using namespace zxy;

char test[3096];

char hexDigit[] = "0123456789ABCDEF";

#define N 1000000

TEST(errlogTest, errlog) {
	for(int i = 0; i != N; ++i)
	{
		lstream << test;
		lstream.reset();
	}

}

TEST(errLogTest, plain) {
	char buf[4096];
	
	for(int i = 0; i != N; ++i)
	{
		snprintf(buf, sizeof buf, "%s\n",
				test);
		//fputs(buf, stderr);
	}

}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);

	unsigned i = 0;
	for ( ; i < sizeof(test) - 1; ++i)
		test[i] = hexDigit[i % 16];
	test[i] = 0;
	return RUN_ALL_TESTS();
}
