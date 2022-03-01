#include "process.h"

namespace unix {

bool Process::Fork(ParentCallback p, ChildCallback c)
{
  pid_ = ::fork();

  // FIXME Exception handling of p and c
  // parent
  if (pid_ > 0) {
    p();
  }
  else if (pid_ == 0) {
  // child
    c();
  }
  else {
    return false;
  }

  return true;
}


} // namespace process