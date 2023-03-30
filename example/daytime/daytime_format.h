#ifndef KANON_EXAMPLE_DAYTIME_FORMAT_H
#define KANON_EXAMPLE_DAYTIME_FORMAT_H

#include <string>

char const* ToMonthStr(int m) KANON_NOEXCEPT;
char const* ToWeekdayStr(int w) KANON_NOEXCEPT;

std::string GetDaytime();

#endif