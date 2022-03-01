#ifndef KANON_HTTP_PIPE_H
#define KANON_HTTP_PIPE_H

#include <stddef.h>
#include <stdexcept>

#include "kanon/util/noncopyable.h"
#include "exception_macro.h"

namespace unix {

DEFINE_EXCEPTION_FROM_OTHER(PipeException, std::runtime_error);

class Pipe : kanon::noncopyable {
public:
  Pipe();

  template<size_t N>
  ssize_t Read(char(&buf)[N])
  { return Read(buf, N); }

  template<size_t N>
  ssize_t Write(char const(&buf)[N])
  { return Write(buf, N); }

  ssize_t Read(void* buf, size_t n);
  ssize_t Write(void const* buf, size_t n);

  void CloseReadEnd();
  void CloseWriteEnd();

  void RedirectReadEnd(int fd);
  void RedirectWriteEnd(int fd);
private:
  int GetReadEnd() const noexcept { return fd_[0]; }
  int GetWriteEnd() const noexcept { return fd_[1]; }

  int fd_[2];
};

} // namespace unix

#endif // KANON_HTTP_PIPE_H