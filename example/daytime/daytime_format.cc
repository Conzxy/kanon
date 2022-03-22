#include "daytime_format.h"

#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

char const* ToMonthStr(int m) noexcept
{
  switch (m) {
    case 1:
      return "January";
    case 2:
      return "February";
    case 3:
      return "March";
    case 4:
      return "April";
    case 5:
      return "May";
    case 6:
      return "June";
    case 7:
      return "July";
    case 8:
      return "August";
    case 9:
      return "September";
    case 10:
      return "October";
    case 11:
      return "November";
    case 12:
      return "December";
    default:
      return "Invalid month number";
  }
}

char const* ToWeekdayStr(int w) noexcept
{
  switch (w) {
    case 0:
      return "Sunday";
    case 1:
      return "Monday";
    case 2:
      return "Tuesday";
    case 3:
      return "Wednesday";
    case 4:
      return "Friday";
    case 5:
      return "Saturday";
    default:
      return "Invalid Weekday";
  }
}

std::string GetDaytime()
{
  std::string daytime;
  daytime.reserve(42);

  time_t sec = time(NULL);
  struct tm* local_time = localtime(&sec);
  
  char buf[256];
  ::strftime(buf, sizeof buf, "%w", local_time);
  
  int weekday_number = buf[0] - '0';
  daytime = ToWeekdayStr(weekday_number);
  daytime += ", ";

  ::strftime(buf, sizeof buf, "%m", local_time);
  int month_number = -1;

  if (buf[0] == '0') {
    month_number = buf[1] - '0';
  }
  else if (buf[0] == '1') {
    month_number = 10 + buf[1] - '0';
  }
  
  daytime += ToMonthStr(month_number); 

  const auto len = ::strftime(buf, sizeof buf, " %d, %Y %H:%M:%S-", local_time);
  daytime.append(buf, len);

  tzset();  
  
  daytime += tzname[0];
  daytime += '\n';

  return daytime;
}