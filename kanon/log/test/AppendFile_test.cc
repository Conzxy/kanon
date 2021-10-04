#include "log/AppendFile.h"

using namespace zxy;

int main() {
	AppendFile file("a.txt");

	file.append("sssssssssssssssssssss", 10);
}
