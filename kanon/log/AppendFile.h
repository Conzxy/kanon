#ifndef KANON_APPENDFILE_H
#define KANON_APPENDFILE_H

#include <stdio.h>
#include "kanon/util/noncopyable.h"
#include "kanon/string/string_view.h"
#include "kanon/util/macro.h"


namespace kanon {

class AppendFile : noncopyable
{
public:
  explicit AppendFile(StringArg filename);
  ~AppendFile() KANON_NOEXCEPT;
  
  void append(char const* data, size_t num) KANON_NOEXCEPT;

  void flush() KANON_NOEXCEPT
  { ::fflush_unlocked(fp_); }

  size_t writtenBytes() const KANON_NOEXCEPT
  { return writtenBytes_; }

private:
  size_t write(char const* data, size_t num) KANON_NOEXCEPT
  { return ::fwrite(data, 1, num, fp_); }

protected:
  char buf_[64 * 1024];
  // indicator of roll file
  size_t writtenBytes_;
  FILE* fp_;

};

} // namespace kanon


#endif // KANON_APPENDFILE_H
