#include "../lexical-cast.h"
#include <gtest/gtest.h>

using namespace kanon;

TEST(LexicalCastTest, Int) {
	char buf[32];
	strcpy(buf, lexical_cast<char const*>(322222222));
	EXPECT_EQ(0, strcmp(buf, "322222222"));
}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
