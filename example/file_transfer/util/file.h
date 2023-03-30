#ifndef KANON_HTTP_FILE_DESCRIPTOR_H
#define KANON_HTTP_FILE_DESCRIPTOR_H

#include <stdio.h>
#include <string>
#include <stddef.h>
#include <stdexcept>

#include "kanon/util/noncopyable.h"

namespace util {

struct FileException : std::runtime_error {
  explicit FileException(char const* msg)
    : std::runtime_error(msg)
  {
  }

  explicit FileException(std::string const& msg) 
    : std::runtime_error(msg)
  {
  }
};

class File : kanon::noncopyable {
public:
  enum OpenMode {
    kRead = 0x1,
    kWriteBegin = 0x2,
    kTruncate = 0x4,
    kAppend = 0x8,
  };

  File()
    : fp_(NULL)
    , mode_(0)
    , eof_(false)
  { }

  File(char const* filename, int mode = kRead);
  File(std::string const& filename, int mode = kRead);
  ~File() KANON_NOEXCEPT;

  bool Open(std::string const& filename, int mode = kRead)
  {
    return Open(filename.c_str(), mode);
  }

  bool Open(char const* filename, int mode = kRead);

  size_t Read(void* buf, size_t len);
  bool ReadLine(std::string& line, const bool need_newline = true);

  bool Write(char const* buf, size_t len) KANON_NOEXCEPT;

  bool IsValid() const KANON_NOEXCEPT { return fp_ != NULL; }
  bool IsEof() const KANON_NOEXCEPT { return eof_; }

  void Rewind() KANON_NOEXCEPT { ::rewind(fp_); }
  void SeekCurrent(long offset) KANON_NOEXCEPT { Seek(offset, SEEK_CUR); }
  void SeekBegin(long offset) KANON_NOEXCEPT { Seek(offset, SEEK_SET); }
  void SeekEnd(long offset) KANON_NOEXCEPT { Seek(offset, SEEK_END); }
  long GetCurrentPosition() KANON_NOEXCEPT { return ::ftell(fp_); }

  size_t GetSize() KANON_NOEXCEPT;

  static const size_t kInvalidReturn = static_cast<size_t>(-1);

private:
  void Seek(long offset, int whence) KANON_NOEXCEPT { ::fseek(fp_, offset, whence); }

  FILE* fp_;
  int mode_;
  bool eof_;
};


} // namespace util

#endif // KANON_HTTP_FILE_DESCRIPTOR_H