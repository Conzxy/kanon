#ifndef KANON_HTTP_FILE_DESCRIPTOR_H
#define KANON_HTTP_FILE_DESCRIPTOR_H

#include <stdio.h>
#include <string>
#include <stddef.h>
#include <stdexcept>

#include "kanon/util/noncopyable.h"

namespace http {

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

  File(char const* filename, int mode);
  File(std::string const& filename, int mode);

  bool Open(std::string const& filename, int mode)
  {
    return Open(filename.data(), mode);
  }

  bool Open(char const* filename, int mode);

  ~File() noexcept;

  size_t Read(void* buf, size_t len);

  template<size_t N>
  size_t Read(char(&buf)[N])
  {
    return Read(buf, N);
  }

  // void Write(char const* buf, size_t len) noexcept;

  // void Append(char const* buf, size_t len) noexcept;

  // template<size_t N>
  // void Write(char const(&buf)[N]) noexcept
  // {
  //   Write(buf, N);
  // }

  // template<size_t N>
  // void Append(char const(&buf)[N]) noexcept
  // {
  //   Append(buf, N);
  // }

  bool IsValid() const noexcept { return fp_ != NULL; }

  bool IsEof() const noexcept { return eof_; }

  void Reset() noexcept { eof_ = false; }

  static const size_t kInvalidReturn = static_cast<size_t>(-1);
private:
  FILE* fp_;
  int mode_;
  bool eof_;
};


} // namespace http

#endif // KANON_HTTP_FILE_DESCRIPTOR_H