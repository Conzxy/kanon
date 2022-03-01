#include <unistd.h>

#include "pipe.h"

namespace unix {

Pipe::Pipe() 
{
  if (::pipe(fd_) < 0) {
    throw PipeException("Call pipe() error occurred");
  }
}

ssize_t Pipe::Read(void* buf, size_t n)
{
  // readn
  auto p = reinterpret_cast<char*>(buf);
  ssize_t remaining = n;
  ssize_t readn = 0;

  while (remaining > 0) {
    readn = ::read(GetReadEnd(), p, remaining);
    if (readn < 0) {
      if (errno == EINTR) {
        continue;
      }
      else {
        return -1;
      }
    } 
    else if (readn == 0) { // EOF
      break;
    }

    remaining -= readn;
    p += readn;
  }

  return static_cast<ssize_t>(n - remaining);
}

ssize_t Pipe::Write(void const* buf, size_t n)
{
  // writen
  auto p = reinterpret_cast<char const*>(buf);
  ssize_t writen = 0;
  size_t remaining = n;

  while (remaining > 0) {
    writen = ::write(GetWriteEnd(), p, remaining);

    if (writen <= 0) {
      if (errno == EINTR) {
        continue;
      }
      else {
        return -1;
      }
    }

    p += writen;
    remaining -= writen;
  }

  return static_cast<ssize_t>(n);
}

void Pipe::CloseReadEnd() 
{ 
  ::close(GetReadEnd()); 
}

void Pipe::CloseWriteEnd()
{
  ::close(GetWriteEnd());
}

void Pipe::RedirectReadEnd(int fd)
{
  ::dup2(GetReadEnd(), fd);
}

void Pipe::RedirectWriteEnd(int fd)
{
  ::dup2(GetWriteEnd(), fd);
}

} // namespace unix