#include "HttpGlobal.h"

using namespace http;

char const* const
http::HttpMethodStrings[HTTP_METHOD_NUM] = 
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
http::HttpVersionStrings[HTTP_VERSION_NUM] = 
{
  "HTTP/1.0",
  "HTTP/1.1",
  "Not Support",
};


int 
http::HttpStatusCodes[] = {
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
http::HttpStatusCodeStrings[] = {
  "Continue",
  "OK",
  "MovedPermanently",
  "NotModified",
  "MovedTemporarily",
  "BadRequest",
  "Unauthorized",
  "Forbidden",
  "NotFound",
  "MethodNotAllowd",
  "NotAcceptable",
  "ProxyAuthenticationRequired",
  "RequestTimeOut",
  "Conflict",
  "LengthRequired",
  "UnsupportedMediaType",
  "InternalServerError",
  "NotImplemeted",
  "ServerUnavailable",
  "HttpVersionNotSupported",
};
