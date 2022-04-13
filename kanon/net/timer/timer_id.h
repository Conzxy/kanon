#ifndef KANON_NET_TIMER_TIMERID_H
#define KANON_NET_TIMER_TIMERID_H

#include "kanon/net/timer/timer.h"

namespace kanon {

//! \addtogroup timer
//!@{

/**
 * \brief A Timer* wrapper
 * \note
 *   This class is exposed to user and has value semantic.
 */
class TimerId {
  friend class TimerQueue;

public:
    TimerId() = default;

    TimerId(Timer* timer)
        : timer_{ timer }
        , seq_{ timer->sequence() }
    { }

private:
    Timer* timer_;
    uint64_t seq_;        
};

//!@}
} // namespace kanon

#endif // KANON_NET_TIMER_TIMERID_H
