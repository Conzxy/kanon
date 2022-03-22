#ifndef KANON_EXAMPLE_DAYTIME_FORMAT_H
#define KANON_EXAMPLE_DAYTIME_FORMAT_H

#include <string>

char const* ToMonthStr(int m) noexcept;
char const* ToWeekdayStr(int w) noexcept;

std::string GetDaytime();

#endif