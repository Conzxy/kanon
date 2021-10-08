#include "kanon/net/timer/TimerQueue.h"
#include "kanon/net/EventLoop.h"
#include "kanon/log/Logger.h"
#include "kanon/util/macro.h"

#include <unistd.h>
#include <sys/timerfd.h>

namespace kanon {

namespace detail {

static inline int createTimerfd() noexcept {
	auto timerfd = ::timerfd_create(CLOCK_MONOTONIC, 
								  TFD_NONBLOCK | TFD_CLOEXEC);

	LOG_TRACE << "tiemrfd: " << timerfd << " created";

	if (timerfd < 0)
		LOG_SYSERROR << "timer_create() error occurred";

	return timerfd;
}

static struct timespec getTimeFromNow(TimeStamp time) noexcept {
	int interval = time.microsecondsSinceEpoch() - TimeStamp::now().microsecondsSinceEpoch();
	if (interval < 100) interval = 100;

	struct timespec expire;
	expire.tv_sec = static_cast<time_t>(interval / TimeStamp::kMicrosecondsPerSeconds_);
	expire.tv_nsec = static_cast<long>(interval % TimeStamp::kMicrosecondsPerSeconds_ * 1000);

	return expire;	
}

static struct timespec getTimerInterval(double time) noexcept {
	static constexpr int kNanoSecondPerSecond = 1000000000;

	int64_t diff = static_cast<int64_t>(time * kNanoSecondPerSecond);
	
	struct timespec interval;
	interval.tv_sec = static_cast<time_t>(diff / kNanoSecondPerSecond);
	interval.tv_nsec = static_cast<long>(diff % kNanoSecondPerSecond);

	return interval;
}

static inline void print_timespec(struct timespec const& spec) KANON_NOEXCEPT {
	LOG_DEBUG << "second: " << spec.tv_sec
			  << ";nanosecond: " << spec.tv_nsec;
}

static inline void print_itimerspec(struct itimerspec const& spec) {
	LOG_DEBUG << "expiration: " 
			  << "second: " << spec.it_value.tv_sec
			  << ",nanosecond: " << spec.it_value.tv_nsec << ";"
			  << "interval: "
			  << "second: " << spec.it_interval.tv_sec
			  << ",nanosecond: " << spec.it_interval.tv_nsec;
}

static void resetTimerfd(int timerfd, Timer const& timer) noexcept {
	struct itimerspec new_value;
	new_value.it_value = getTimeFromNow(timer.expiration());
	new_value.it_interval = getTimerInterval(timer.interval());
	
	print_itimerspec(new_value);
	if (::timerfd_settime(timerfd, 0, &new_value, NULL))
		LOG_SYSERROR << "timerfd_settime error occurred";

	LOG_TRACE << "reset successfully";
}

static void readTimerfd(int timerfd) noexcept {
	uint64_t dummy = 0;
	uint64_t n;

	if ((n = ::read(timerfd, &dummy, sizeof dummy)) != sizeof dummy)
		LOG_SYSERROR << "timerfd read error";

	LOG_TRACE << "read " << n << " bytes";
}

} // namespace detail

TimerQueue::TimerQueue(EventLoop* loop)
	: timerfd_{ detail::createTimerfd() }
	, timer_channel_{ kanon::make_unique<Channel>(loop, timerfd_) }
	, loop_{ loop }
{
    timer_channel_->set_read_callback([this](){
		loop_->assertInThread();

		TimeStamp now{ TimeStamp::now() };
		detail::readTimerfd(timerfd_);	

		auto expired_timers = this->getExpiredTimers(now);
		
		for (auto const& timer : expired_timers) {
			timer.second->run();
		}
		
		for (auto const& timer : expired_timers) {
			if (!timer.second->repeat())
				timer_map_.erase(timer.first);
		}	

		this->reset(expired_timers, now);
	});

	timer_channel_->set_error_callback([](){
		LOG_ERROR << "timer event handle error occurred";
	});

	timer_channel_->enableReading();
}


TimerId TimerQueue::addTimer(Timer::TimerCallback cb, 
							 TimeStamp time,
							 double interval) {
	auto ptimer = new Timer{ std::move(cb), time, interval };

	loop_->runInLoop([this, ptimer]() {
			loop_->assertInThread();
			
			bool earliest_update = this->emplace(ptimer);

			if (earliest_update)
				kanon::detail::resetTimerfd(timerfd_, *ptimer);
	});

	return { ptimer };
}

bool TimerQueue::emplace(Timer* ptimer) {
	std::unique_ptr<Timer> uptimer{ ptimer };

	assert(timer_map_.size() == active_timer_set_.size());
	
	auto earliest_timer_iter = timer_map_.begin();
	auto ret = false;

	if (timer_map_.empty() || 
		ptimer->expiration() < earliest_timer_iter->first) {
		ret = true;
	}

	{
		timer_map_.emplace(
				std::make_pair(
					ptimer->expiration(),
					std::move(uptimer)));
	}
	{
		active_timer_set_.emplace(
				std::make_pair(
					ptimer,
					ptimer->sequence()));
	}

	LOG_TRACE << "now timer total num: " << timer_map_.size();	
	return ret;
}

auto TimerQueue::getExpiredTimers(TimeStamp time) 
	-> TimerVector {
	assert(!timer_map_.empty());
	
	TimerVector expireds;

	auto expired_end = timer_map_.lower_bound(time);

	assert(expired_end == timer_map_.end() || time < expired_end->first);
	
	for (auto timer = timer_map_.begin();
		 timer != expired_end;
		 ++timer) {
		auto ptimer{ timer->second.get() };
		if (ptimer->repeat()) {
			ptimer->restart(time);
		}
		else {
			//timer_map_.erase(timer);
			active_timer_set_.erase(ActiveTimer{ 
					ptimer,
					ptimer->sequence() });
		}
		expireds.emplace_back(TimerEntry{ timer->first, timer->second.get() });
	}

	//std::copy(std::make_move_iterator(timer_map_.begin()), 
			  //std::make_move_iterator(expired_end), 
			  //std::back_inserter(expireds));

	//for (auto const& timer : expireds) {
		//if (!timer.second->repeat())
			//active_timer_set_.erase(ActiveTimer{ 
					//timer.second.get(), 
					//timer.second.get()->sequence() 
		//});
	//}
	
	return expireds;		
}

void 
TimerQueue::reset(TimerVector& expireds, TimeStamp now) {
	KANON_UNUSED(expireds);
	KANON_UNUSED(now);

	Timer* next_expire{ nullptr };

	if (!timer_map_.empty()) {
		next_expire = timer_map_.begin()->second.get();
	}

	if (next_expire) {
		detail::resetTimerfd(timerfd_, *next_expire);
	}
}

} // namespace kanon
