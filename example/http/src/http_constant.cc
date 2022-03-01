#include "http_constant.h"

namespace http {
namespace detail {

char const* const
http_method_strings[HTTP_METHOD_NUM] = 
{
  "NotSupport",
  "GET",
  "POST",
  "PUT",
  "DELETE",
  "TRACE",
  "HEAD",
  "CONNECT",
  "OPTIONS",
};

char const* const
http_version_strings[HTTP_VERSION_NUM] = 
{
  "HTTP/1.0",
  "HTTP/1.1",
  "Not Support",
};


int 
http_status_code[] = {
  100,
  200,
  301,
  304,
  307,
  400,
  401,
  403,
  404,
  405,
  406,
  407,
  408,
  409,
  411,
  415,
  500,
  501,
  503,
  505
};

char const*
http_status_code_strings[] = {
  "Continue",
  "OK",
  "Moved Permanently",
  "Not Modified",
  "Moved Temporarily",
  "Bad Request",
  "Unauthorized",
  "Forbidden",
  "Not Found",
  "Method Not Allowd",
  "Not Acceptable",
  "Proxy Authentication Required",
  "Request TimeOut",
  "Conflict",
  "Length Required",
  "Unsupported MediaType",
  "Internal ServerError",
  "Not Implemeted",
  "Server Unavailable",
  "Http Version Not Supported",
};

} // namespace detail
} // namespace http