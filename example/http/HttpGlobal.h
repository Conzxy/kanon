#ifndef KANON_HTTP_GLOBAL_H
#define KANON_HTTP_GLOBAL_H

#include <map>
#include <assert.h>

namespace http {

/**
 * @enum HttpMethod
 * @brief 
 * Express http method
 */
enum class HttpMethod {
  kNotSupport = 0,
  kGet,
  kPost,
  kPut,
  kDelete,
  kTrace,
  kHead,
  kConnect,
  kOptions,
  kNum
};

#define HTTP_METHOD_NUM \
  static_cast<unsigned long>(HttpMethod::kNum) 

/** Map method enum to corresponding description string */
extern char const* const 
HttpMethodStrings[HTTP_METHOD_NUM];

/**
 * @enum HttpStatusCode
 * @brief 
 * Expresss staus code which used in http response line
 */
enum class HttpStatusCode {
  k100Continue = 0,
  k200OK,
  k301MovedPermanently,
  k304NotModified,
  k307MovedTemporarily,
  k400BadRequest,
  k401Unauthorized,
  k403Forbidden,
  k404NotFound,
  k405MethodNotAllowd,
  k406NotAcceptable,
  k407ProxyAuthenticationRequired,
  k408RequestTimeOut,
  k409Conflict,
  k411LengthRequired,
  k415UnsupportedMediaType,
  k500InternalServerError,
  k501NotImplemeted,
  k503ServerUnavailable,
  k505HttpVersionNotSupported,
  kNum,
};


#define HTTP_STATUS_CODE_NUM \
  static_cast<unsigned long>(HttpStatusCode::kNum)

extern char const*
HttpStatusCodeStrings[HTTP_STATUS_CODE_NUM];

extern int
HttpStatusCodes[HTTP_STATUS_CODE_NUM];

/** Map status code to corresponding description string */
inline int getStatusCode(HttpStatusCode code) {
  return HttpStatusCodes[(unsigned)code];
}

inline char const* getStatusCodeString(HttpStatusCode code) {
  return HttpStatusCodeStrings[(unsigned)code];
}

/**
 * @enum HttpVersion
 * @brief 
 * Express http version code which server can support
 */
enum class HttpVersion {
  kHttp10 = 0,
  kHttp11,
  kNotSupport,
  kNum
};

#define HTTP_VERSION_NUM \
  static_cast<unsigned long>(HttpVersion::kNum)

/** Map http version code to corresponding description string */
extern char const* const
HttpVersionStrings[HTTP_VERSION_NUM];

} // namespace http

#endif // KANON_HTTP_GLOBAL_H
