#include "kanon/net/timer/Timer.h"

namespace kanon {

AtomicInt64 Timer::s_counter_ {};

void
Timer::restart(TimeStamp now) KANON_NOEXCEPT {
	if (repeat_) {
		expiration_ = addTime(now, interval_);
	} else {
		expiration_.
	}
} // namespace kanon
