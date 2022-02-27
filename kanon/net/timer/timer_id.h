#ifndef KANON_NET_TIMER_TIMERID_H
#define KANON_NET_TIMER_TIMERID_H

#include "kanon/net/timer/timer.h"

namespace kanon {

class TimerQueue;

/**
 * @brief since this class is exposed to user,
 *        not just a struct and own value semantic
 */
class TimerId /* : public copyable */ {
  friend class TimerQueue;

public:
    TimerId()
        : timer_{ NULL }
        , seq_{ 0 }
    { }

    TimerId(Timer* timer)
        : timer_{ timer }
        , seq_{ timer->sequence() }
    { }

private:
    Timer* timer_;
    int seq_;
};

} // namespace kanon

#endif // KANON_NET_TIMER_TIMERID_H
