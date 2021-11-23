#ifndef KANON_HTTP_RESPONSE_H
#define KANON_HTTP_RESPONSE_H

#include "HttpGlobal.h"

#include "kanon/util/noncopyable.h"
#include "kanon/net/Buffer.h"
#include "kanon/util/type.h"

namespace http {

/**
 * @class HttpResponse
 * @brief
 * build a http response
 */
class HttpResponse {
public:
  explicit HttpResponse()
    : buffer_{}
  { }

  /**
   * @brief 
   * Add version code in response line.
   * Only support HTTP1.0 and HTTP1.1.
   * @param ver valid http version code
   */
  void addVersion(HttpVersion ver) {
    buffer_.append(HttpVersionStrings[(int)ver]);
  }

  /**
   * @brief 
   * Add status code in response line
   * and corresponding description string.
   * @param code valid status code
   */ 
  void addStatusCodeAndString(HttpStatusCode code) {
    char buf[256];
    snprintf(buf, sizeof buf, " %d %s\r\n",
        getStatusCode(code), getStatusCodeString(code));
    
    buffer_.append(static_cast<char const*>(buf));

  }

  /**
   * @brief
   * Add header in response line
   * including its field and content.
   * @param field field name
   * @param content corresponding description
   */
  void addHeader(kanon::StringView field,
      kanon::StringView content) {
    char buf[256];

    snprintf(buf, sizeof buf, "%.*s: %.*s\r\n", 
        field.size(), field.data(), content.size(), content.data());

    buffer_.append(buf);
  }


  /**
   * @brief
   * Add blankline after header lines.
   * It should be \r\n\r\n(other may be invalid to browser)
   */
  void addBlankLine() {
    buffer_.append("\r\n\r\n");
  }

  /**
   * @brief
   * Add body contents.
   * @param content body content
   */
  void addBody(kanon::StringView content) {
    buffer_.append(content);
  }
  
  kanon::Buffer& buffer() noexcept
  { return buffer_; }
  
private:
  // Temp buffer
  // It should passed to send(Buffer&)
  kanon::Buffer buffer_;
};

HttpResponse clientError(HttpStatusCode status_code,
    kanon::StringView msg);
} // namespace http

#endif // KANON_HTTP_RESPONSE_H
