#include "../string-builder.h"
#include "../string-builder_.h"
#include <gtest/gtest.h>
#include <iostream>

using namespace zxy;
using namespace std;

#define N 1000000

char const* kTestStr = "123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						"123456789ABCDEF"
						;


TEST(stringCat, plain){
	string s1 = kTestStr;
	
	for(unsigned i = 0; i != N; ++i)
		string s = s1 + s1 + s1 + s1 + s1 + s1 + s1 + s1 + s1 + s1;
}

TEST(stringCat, builder_){
	StringWrapper s1 = kTestStr;

	for(unsigned i = 0; i != N; ++i){
		auto s = s1 + s1 + s1 + s1 + s1 + s1 + s1 + s1 + s1 + s1;
		(void)s.convert();

	}
}

TEST(stringCat, builder){
	String s1 = kTestStr;
	
	for(unsigned i = 0; i != N; ++i)
	{
		String s = s1 + s1 + s1 + s1 + s1 + s1 + s1 + s1 + s1 + s1;
		//cout << (string)s << '\n';
	}
	
}


int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
