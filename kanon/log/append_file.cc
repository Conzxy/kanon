#include "kanon/log/append_file.h"

#include "kanon/util/macro.h"

#ifdef KANON_ON_UNIX
#include <sys/stat.h>
#include <unistd.h>
#elif defined(KANON_ON_WIN)
#include <direct.h>
#endif

#include "kanon/log/logger.h"

namespace kanon {

AppendFile::AppendFile(StringArg filename)
  : writtenBytes_(0)
{
  while (!(fp_ = ::fopen(filename.data(), "a"))) {
    if (errno != ENOENT) {
      ::fprintf(stderr, "Failed to open file: %s\n", filename.data());
      ::fflush(stderr);
      ::abort();
    }

    auto filename_view = StringView(filename.data());
    const std::string dir =
        filename_view.substr(0, filename_view.rfind('/')).ToString();
    auto ret = 0;

#ifdef KANON_ON_UNIX
    ret = ::mkdir(dir.data(), S_IRUSR);
#elif defined(KANON_ON_WIN)
    ret = ::_mkdir(dir.data());
#else
    // TODO Use c++17 file system API
    ::fprintf(
        "TODO: Call the directory creation function in specific platform!");
    ::abort();
#endif
    if (ret != 0) {
      ::fprintf(stderr, "Failed to create a directory: %s\n", dir.data());
      ::fflush(stderr);
      ::abort();
    }
  }

  setvbuf(fp_, buf_, _IOFBF, sizeof buf_);
}

AppendFile::~AppendFile() noexcept
{
  Flush();
  if (fp_) ::fclose(fp_);
}

void AppendFile::_Append(char const *data, size_t num) noexcept
{
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
