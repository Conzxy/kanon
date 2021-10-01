#include "../string-view.h"
#include <gtest/gtest.h>
#include <iostream>
#include <iterator>

using namespace zxy;
using namespace std;
using namespace zxy::literal;

StringView sv = "B AB AB B ";
StringView empty_sv = "";

#define NPOS zxy::StringView::npos

TEST(StringViewTest, sizeTest){
	cout << sv.size() << '\n';
}

TEST(StringViewTest, streamTest){
	cout << sv << '\n';
}

TEST(StringViewTest, emptyTest){
	EXPECT_EQ(true, empty_sv.empty());	
}

TEST(StringViewTest, findTest){
	StringView v = "long long int";
	EXPECT_EQ(5, v.find("long", 2));
}

TEST(StringViewTest, rfindTest){
	EXPECT_EQ(NPOS, sv.rfind("AB", 1));
	EXPECT_EQ(2, sv.rfind("AB", 2));
}

TEST(StringViewTest, compareTest){
	EXPECT_EQ(true, "abd"_sv > "abcd"_sv);
}

TEST(StringViewTest, starts_withTest){
	EXPECT_EQ(true, "tcp/ip"_sv.starts_with("tcp"));
}

TEST(StringViewTest, ends_withTest){
	EXPECT_EQ(true, "std::string"_sv.ends_with("string"));
	EXPECT_EQ(true, "\n"_sv.ends_with("\n"));
}

TEST(StringViewTest, remove_prefixTest){
	auto v = "           aaaa"_sv;
	v.remove_prefix(v.find_first_not_of(" "));
	
	EXPECT_EQ(4, v.size());
	EXPECT_EQ(0, memcmp(v.data(), "aaaa", v.size()));
}

TEST(StringViewTest, remove_suffixTest){
	auto v = "aaaa                  "_sv;
	v.remove_suffix(v.size() - v.find_first_of(" "));

	EXPECT_EQ(4, v.size());
	EXPECT_EQ(0, memcmp(v.data(), "aaaa", v.size()));
}

TEST(StringViewTest, find_first_ofTest){
	EXPECT_EQ(1, "alignas"_sv.find_first_of("klmn"_sv));
	EXPECT_EQ(NPOS, "alignas"_sv.find_first_of("wxyz"_sv));
	EXPECT_EQ(6, "constexpr"_sv.find_first_of("x"));
}

TEST(StringViewTest, find_first_not_ofTest){
	EXPECT_EQ(2, "BCDEF"_sv.find_first_not_of("ABC"));
	EXPECT_EQ(4, "BCDEF"_sv.find_first_not_of("ABC", 4));
	EXPECT_EQ(1, "BCDEF"_sv.find_first_not_of("B"));
	EXPECT_EQ(3, "BCDEF"_sv.find_first_not_of("D", 2));
}

TEST(StringViewTest, find_last_not_ofTest){
	EXPECT_EQ(1, "BCDEF"_sv.find_last_not_of("DEF"));
	EXPECT_EQ(2, "BCDEFG"_sv.find_last_not_of("EFG", 3));
	EXPECT_EQ(2, "ABBA"_sv.find_last_not_of("A"));	
	EXPECT_EQ(1, "ABBA"_sv.find_last_not_of("A", 1));
}

TEST(StringViewTest, find_last_ofTest){
	EXPECT_EQ(5, "delete"_sv.find_last_of("cdef"_sv));
	EXPECT_EQ(NPOS, "double"_sv.find_last_of("fghi"_sv));
}

TEST(StringViewTest, split) {
	StringView x = "a b c d";

	auto split = x.split();
	
	std::cout << "split test begin\n";
	for (auto const& e : split) {
		std::cout << e << '\n';
	}
	std::cout << "split test end\n";
}

int main(int argc, char* argv[])
{
	ios::sync_with_stdio(false);
	cout.tie(NULL);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
