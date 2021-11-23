#ifndef KANON_HTTP_TYPES_H
#define KANON_HTTP_TYPES_H

#include <map>

namespace http {

typedef std::map<std::string, std::string> HeaderMap;
typedef HeaderMap::value_type HeaderType;

} // namespace http

#endif // KANON_HTTP_TYPES_H
