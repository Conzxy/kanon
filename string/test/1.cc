#include <string_view>
#include <stdio.h>
using namespace std::literals;

int main() {
	auto x = "aaaaa"sv.find('a');
	
	printf("%d", x);
}
