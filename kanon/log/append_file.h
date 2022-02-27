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
  ~AppendFile() noexcept;
  
  void Append(char const* data, size_t num) noexcept;

  void flush() noexcept
  { ::fflush_unlocked(fp_); }

  size_t writtenBytes() const noexcept
  { return writtenBytes_; }

private:
  size_t write(char const* data, size_t num) noexcept
  { return ::fwrite(data, 1, num, fp_); }

protected:
  char buf_[64 * 1024];
  // indicator of roll file
  size_t writtenBytes_;
  FILE* fp_;

};

} // namespace kanon


#endif // KANON_APPENDFILE_H
