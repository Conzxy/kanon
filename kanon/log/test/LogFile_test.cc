#include "../LogFile.h"
#include "../Logger.h"

using namespace kanon;

int main() {
	Logger::setOutputCallback(LogFile::append);
	Logger::setFlushCallback(LogFile::flush);

	Logger
}
