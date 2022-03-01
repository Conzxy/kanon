#ifndef KANON_HTTP_GLOBAL_H
#define KANON_HTTP_GLOBAL_H

#include <assert.h>

#include "kanon/util/macro.h"

namespace http {

/**
 * @enum HttpMethod
 * Represents http method
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

/**
 * @enum HttpStatusCode
 * Represents status code which used in http response line
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

constexpr HttpStatusCode kInvalidStatusCode = HttpStatusCode::kNum;

#define HTTP_STATUS_CODE_NUM \
  static_cast<unsigned long>(HttpStatusCode::kNum)

/**
 * @enum HttpVersion
 * Represents http version code which server can support
 */
enum class HttpVersion {
  kHttp10 = 0,
  kHttp11,
  kNotSupport,
  kNum
};

#define HTTP_VERSION_NUM \
  static_cast<unsigned long>(HttpVersion::kNum)

namespace detail {

/** Map method enum to corresponding description string */
extern char const* const 
http_method_strings[HTTP_METHOD_NUM];

/** Map status code enum to corresponding description string */
extern char const*
http_status_code_strings[HTTP_STATUS_CODE_NUM];

/** Map status code enum to integer represent */
extern int
http_status_code[HTTP_STATUS_CODE_NUM];

/** Map http version code to corresponding description string */
extern char const* const
http_version_strings[HTTP_VERSION_NUM];

} // namespace detail

/**
 * Transform Status Code to integer represent 
 * FIXME 
 * Don't use kNum and array to get string or integer and so on,
 * Use switch + case to return them.
 */
inline int GetStatusCode(HttpStatusCode code) noexcept {
  KANON_ASSERT((int)code >= 0 && code <= HttpStatusCode::kNum, "Invalid http status code");
  return detail::http_status_code[(unsigned)code];
}

/** Transform method to according description string */
inline char const* GetMethodString(HttpMethod method) noexcept {
  KANON_ASSERT((int)method >= 0 && method <= HttpMethod::kNum, "Invalid http method");
  return detail::http_method_strings[(unsigned)method];
}

/** Transform status code to according description string */
inline char const* GetStatusCodeString(HttpStatusCode code) noexcept {
  KANON_ASSERT((int)code >= 0 && code <= HttpStatusCode::kNum, "Invalid http status code");
  return detail::http_status_code_strings[(unsigned)code];
}

/** Transform http version to according description string */
inline char const* GetHttpVersionString(HttpVersion ver) noexcept {
  KANON_ASSERT((int)ver >= 0 && ver <= HttpVersion::kNum, "Invalid http Version");
  return detail::http_version_strings[(unsigned)ver];
}

} // namespace http

#endif // KANON_HTTP_GLOBAL_H