#include "../lexical-stream.h"
#include <gtest/gtest.h>
#include <sstream>
#include <string.h>

using namespace std;
using namespace kanon;

#define N 1000000

#define LOOP1(stream, val, n) \
	do{ \
		for(unsigned i = 0; i != n; ++i){ \
			 stream << val;	 \
			 stream.reset(); \
		} \
	}while(0)

#define LOOP2(stream, val, n) \
	do{ \
		for(unsigned i = 0; i != n; ++i){ \
			 stream << val;	 \
		} \
	}while(0)


constexpr int a = 322222222;
char const* ac = "322222222";

constexpr double b = 3.222;
char const* c = "aaaaaaaaaaaaaaaaaaaaaaaaa";

TEST(LexicalStreamTest, Int){
	SmallLexicalStream lstream;	
	LOOP1(lstream, a, N);
}

TEST(ostringstreamTest, Int){
	ostringstream oss;
	LOOP2(oss, a, N);
}

TEST(LexicalStreamTest, Double) {
	SmallLexicalStream lstream;
	LOOP1(lstream, b, N);
}

TEST(ostringstreamTest, Double) {
	ostringstream oss;
	LOOP2(oss, b, N);
}

TEST(LexicalStreamTest, String) {
	SmallLexicalStream lstream;
	LOOP1(lstream, c, N);
}

TEST(ostringstreamTest, String) {
	ostringstream oss;
	LOOP2(oss, c, N);
}

TEST(LexicalStreamTest, IntEQ) {
	SmallLexicalStream stream;
	stream << a;
	EXPECT_EQ(0, memcmp(ac, stream.data(), stream.size()));
}

TEST(snprintfTest, Int) {
	char buf[64];
	for(unsigned i = 0; i != N; ++i)
		snprintf(buf, sizeof buf, "%d", a);

}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);	
	return RUN_ALL_TESTS();
	
}
