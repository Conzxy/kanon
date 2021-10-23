#include "AppendFile.h"
#include "Logger.h"

namespace kanon {

AppendFile::AppendFile(StringArg filename)
	: writtenBytes_(0)
	, fp_(::fopen(filename.data(), "a"))
{
	if (! fp_)
		::fprintf(stderr, "failed in open file: %s", filename.data());	
	::setbuffer(fp_, buf_, sizeof buf_);
}

AppendFile::~AppendFile() KANON_NOEXCEPT {
	flush();
	if (fp_)
		::fclose(fp_);
}

void AppendFile::append(char const* data, size_t num) KANON_NOEXCEPT {
	size_t written = 0;

	do {
		size_t left = num - written;
		auto n = write(data + written, left);
		
		if (n != left) {
			auto err = ferror(fp_);
			if (err)
				fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
			break;
		}
		
		written += n;
	} while (written != num);

	writtenBytes_ += written;
}

} // namespace kanon
