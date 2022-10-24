#include "kanon/util/time_stamp.h"

#include <gtest/gtest.h>
#include <iostream>

using namespace kanon;

TEST (time_stamp, operator_add) 
{
  TimeStamp ts = TimeStamp::Now();
  auto ts1 = TimeStamp::Second(10) + ts + TimeStamp::Microsecond(10);
  EXPECT_EQ(ts1.microseconds() - ts.microseconds(), 10 * TimeStamp::kMicrosecondsPerSeconds_ + 10);

  auto ts2 = 10.0 + ts + 10.0 / TimeStamp::kMicrosecondsPerSeconds_;
  EXPECT_EQ(ts2.microseconds() - ts.microseconds(), 10 * TimeStamp::kMicrosecondsPerSeconds_ + 10);

  auto ts3 = 10.0 + ts + TimeStamp::Second(10);
  EXPECT_EQ(ts3.seconds() - ts.seconds(), 20);
}

TEST (time_stamp, operator_minus)
{
  TimeStamp now = TimeStamp::Now();

  TimeStamp ts1 = now - TimeStamp::Second(10) - 9;
  EXPECT_EQ(now.seconds() - ts1.seconds(), 19);

  TimeStamp ts2 = now - TimeStamp::Microsecond(1) - TimeStamp::MilliSecond(1) - TimeStamp::Second(1);
  EXPECT_EQ(now.microseconds() - ts2.microseconds(), 1 + 1000 + 1000000);
}

TEST (time_stamp, operator_reverse)
{
  TimeStamp now = TimeStamp::Now();
  EXPECT_EQ((-now).microseconds(), -now.microseconds());
}

TEST (time_stamp, from_time_str)
{
  // setlocale(LC_ALL, "zh_CN.UTF-8");
  TimeStamp ts = TimeStamp::FromTimeStr("%Y-%m-%d %H:%M:%S", "2022-12-1 18:9:8");
  std::cout << ts.ToFormattedString() << std::endl;

  TimeStamp ts1 = TimeStamp::FromTimeStr("%H:%M:%S", "18:9:8");
  std::cout << ts1.ToFormattedString() << std::endl;
}
