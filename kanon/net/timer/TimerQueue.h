#ifndef KANON_NET_TIMER_TIMERQUEUE_H
#define KANON_NET_TIMER_TIMERQUEUE_H

#include "kanon/util/noncopyable.h"
#include "kanon/time/TimeStamp.h"
#include "kanon/net/timer/Timer.h"
#include "kanon/net/timer/TimerId.h"
#include "kanon/net/Channel.h"
#include "kanon/util/unique_ptr.h"

#include <map>
#include <set>

namespace kanon {
class EventLoop;

class TimerQueue : noncopyable {
public:
	typedef Timer::TimerCallback TimerCallback;
	
	TimerQueue(EventLoop* loop);
	TimerId addTimer(TimerCallback cb,
					 TimeStamp time,
					 double interval);
	
	void cancelTimer(TimerId const& id);
private:
	typedef std::pair<Timer*, int64_t> ActiveTimer;
	typedef std::pair<const TimeStamp, std::unique_ptr<Timer>> TimerEntry;
	typedef std::vector<TimerEntry> TimerVector;
	typedef std::multimap<TimeStamp, std::unique_ptr<Timer>> TimerMap;

	bool emplace(std::unique_ptr<Timer>);
	TimerVector getExpiredTimers(TimeStamp time);

	void reset(TimerVector&, TimeStamp now);
private:
	int timerfd_;
	std::unique_ptr<Channel> timer_channel_;

	TimerMap timer_map_;
	std::set<ActiveTimer> active_timer_set_; // use it to tell timer whether live or not
	
	bool calling_timer_;	
	std::set<ActiveTimer> canceling_timers_;	
	EventLoop* loop_;
};

} // namespace kanon

#endif // KANON_NET_TIMER_TIMERQUEUE_H
