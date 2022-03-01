#ifndef KANON_HTTP_PROCESS_H
#define KANON_HTTP_PROCESS_H

#include <stdexcept>
#include <unistd.h>
#include <functional>

#include "kanon/util/noncopyable.h"
#include "exception_macro.h"

namespace unix {

DEFINE_EXCEPTION_FROM_OTHER(ProcessException, std::runtime_error);

class Process : kanon::noncopyable {
public:
  using ParentCallback = std::function<void ()>;
  using ChildCallback = ParentCallback;

  Process()
    : pid_(-1)
  { 
  }

  bool Fork(ParentCallback p, ChildCallback c);

  pid_t GetPid() const noexcept { return pid_; }
private:
  pid_t pid_;
};

}

#endif // KANON_HTTP_PROCESS_H