#include "kanon/net/timer/timer.h"

namespace kanon {

AtomicCounter64 Timer::s_counter_ {};

void
Timer::restart(TimeStamp now) noexcept {
  if (repeat_) {
    expiration_ = AddTime(now, interval_);
  } else {
    expiration_.ToInvalid();
  }
}

} // namespace kanon
