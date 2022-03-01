#ifndef KANON_HTTP_EXCEPTION_MACRO_H
#define KANON_HTTP_EXCEPTION_MACRO_H

#include <stdexcept>

#define DEFINE_EXCEPTION_FROM_OTHER(exception, other) \
class exception : public other {\
public:\
  explicit exception(std::string const& str)\
    : other(str)\
  { }\
\
  explicit exception(char const* str)\
    : other(str)\
  { }\
}

#endif // KANON_HTTP_EXCEPTOIN_MACRO_H

