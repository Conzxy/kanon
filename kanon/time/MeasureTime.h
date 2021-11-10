#ifndef MEASURE_TIME_H
#define MEASURE_TIME_H

#include "kanon/string/string-view.h"
#include "kanon/util/macro.h"

#include <chrono>
#include <functional>
#include <utility>
#include <ctime>
#include <stdexcept>

#define CHRONO std::chrono

namespace kanon {

#define TIME_INTERVAL \
	(CHRONO::high_resolution_clock::now() - begin_)
#define DURATION_CAST(timetype, d) \
	(CHRONO::duration_cast<CHRONO::timetype>(d))
#define MEASURE_CLOCK \
	 CHRONO::high_resolution_clock

enum TimeType
{
	seconds,
	milliseconds,
	microseconds,
	nanoseconds,
};

class MeasureTime
{
public:
	using count_t = unsigned long long;

	explicit MeasureTime(StringView func_name, TimeType ty=TimeType::nanoseconds)
		: func_name_{func_name},
		  begin_{MEASURE_CLOCK::now()},
		  time_t_{ty}
	{};
	
	void reset()
	{
		begin_ = MEASURE_CLOCK::now();
	}

	~MeasureTime()
	{
		switch(time_t_)
		{
			case TimeType::seconds:
				printTime(DURATION_CAST(seconds, TIME_INTERVAL));break;
			case TimeType::milliseconds:
				printTime(DURATION_CAST(milliseconds, TIME_INTERVAL));break;
			case TimeType::microseconds:
				printTime(DURATION_CAST(microseconds, TIME_INTERVAL));break;
			case TimeType::nanoseconds:
				printTime(DURATION_CAST(nanoseconds, TIME_INTERVAL));break;
		}
	}

	static count_t diff(MEASURE_CLOCK::time_point const& left,
						MEASURE_CLOCK::time_point const& right,
						TimeType ty=TimeType::nanoseconds)
	{
		auto interval = right - left;
		switch(ty)
		{
			case TimeType::seconds:
				return static_cast<count_t>(DURATION_CAST(seconds, interval).count());break;
			case TimeType::milliseconds:
				return static_cast<count_t>(DURATION_CAST(milliseconds, interval).count());break;
			case TimeType::microseconds:
				return static_cast<count_t>(DURATION_CAST(microseconds, interval).count());break;
			case TimeType::nanoseconds:
				return static_cast<count_t>(DURATION_CAST(nanoseconds, interval).count());break;
		}
	}
	
private:
	void printTime(CHRONO::seconds const& dura) const
	{
		printf("Function { %s }: %llus\n", func_name_.data(),
				static_cast<count_t>(dura.count()));
	}
	
	void printTime(CHRONO::milliseconds const& dura) const
	{
		printf("Function { %s }: %llums\n", func_name_.data(),
				static_cast<count_t>(dura.count()));	
	}

	void printTime(CHRONO::microseconds const& dura) const
	{
		printf("Function { %s }: %lluus\n", func_name_.data(), 
				static_cast<count_t>(dura.count()));
	}

	void printTime(CHRONO::nanoseconds const& dura) const
	{
		printf("Function { %s }: %lluns\n", func_name_.data(), 
				static_cast<count_t>(dura.count()));
	}

private:
	StringView func_name_;
	MEASURE_CLOCK::time_point begin_;
	TimeType time_t_;
};

template<int TY=TimeType::nanoseconds, typename Func, typename ...Args>
inline void measureTime(Func&& func, StringView func_name, Args&&... args) {
  auto dummy = MeasureTime{ func_name, static_cast<TimeType>(TY) };
  KANON_UNUSED(dummy);

  std::forward<Func>(func)(std::forward<Args>(args)...); 
}

#define MEASURE_TIME(func, ...) \
  measureTime(func, #func, ##__VA_ARGS__)

} // namespace kanon

#endif // MEASURE_TIME_H
