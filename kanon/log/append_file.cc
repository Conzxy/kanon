#include "kanon/log/append_file.h"

#include <sys/stat.h>
#include <unistd.h>

#include "kanon/log/logger.h"

namespace kanon {

AppendFile::AppendFile(StringArg filename)
  : writtenBytes_(0)
{
  while (! (fp_ = ::fopen(filename.data(), "a") ) ) {
    if (errno != ENOENT) {
      ::fprintf(stderr, "Failed to open file: %s\n", filename.data());  
      ::fflush(stderr);
      ::abort();
    }

    auto filename_view = StringView(filename.data());
    const std::string dir = filename_view.substr(0, filename_view.rfind('/')).ToString();

    if (::mkdir(dir.data(), S_IRUSR) ) {
      ::fprintf(stderr, "Failed to create a directory: %s\n", dir.data());  
      ::fflush(stderr);
      ::abort();
    }
  }

  ::setbuffer(fp_, buf_, sizeof buf_);
}

AppendFile::~AppendFile() noexcept {
  Flush();
  if (fp_)
    ::fclose(fp_);
}

void AppendFile::Append(char const* data, size_t num) noexcept {
  size_t written = 0;

  do {
    size_t left = num - written;
    auto n = write(data + written, left);
    
    if (n != left) {
      auto err = ferror(fp_);
      if (err)
        fprintf(stderr, "AppendFile::Append() failed %s\n", strerror_tl(err));
      break;
    }
    
    written += n;
  } while (written != num);

  writtenBytes_ += written;
}

} // namespace kanon
