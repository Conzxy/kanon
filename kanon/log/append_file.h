#ifndef KANON_APPENDFILE_H
#define KANON_APPENDFILE_H

#include <stdio.h>

#include "kanon/string/string_view.h"
#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"

namespace kanon {

class AppendFile : noncopyable {
 public:
  KANON_CORE_API explicit AppendFile(StringArg filename);
  KANON_CORE_API ~AppendFile() KANON_NOEXCEPT;

  void Append(void const *data, size_t num) KANON_NOEXCEPT
  {
    _Append((char const *)data, num);
  }

  void Flush() KANON_NOEXCEPT
  {
#if defined(__GUNC__)
    ::fflush_unlocked(fp_);
#else
    ::fflush(fp_);
#endif
  }

  size_t writtenBytes() const KANON_NOEXCEPT { return writtenBytes_; }

  FILE *fp() const KANON_NOEXCEPT { return fp_; }

 private:
  size_t write(char const *data, size_t num) KANON_NOEXCEPT
  {
    return ::fwrite(data, 1, num, fp_);
  }

  KANON_CORE_API void _Append(char const *data, size_t num) KANON_NOEXCEPT;

 protected:
  char buf_[64 * 1024];
  // indicator of roll file
  size_t writtenBytes_;
  FILE *fp_;
};

} // namespace kanon

#endif // KANON_APPENDFILE_H
