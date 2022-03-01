#ifndef KANON_HTTP_RESPONSE_H
#define KANON_HTTP_RESPONSE_H

#include "http_constant.h"

#include "kanon/string/string_view.h"
#include "kanon/util/macro.h"
#include "kanon/util/mem.h"
#include "kanon/util/noncopyable.h"
#include "kanon/net/buffer.h"
#include "kanon/util/type.h"

namespace http {

/**
 * Construct a http response
 */
class HttpResponse {
public:
  using Self = HttpResponse;

  explicit HttpResponse()
    : buffer_()
    , body_()
  { }

  Self& AddHeaderLine(HttpStatusCode code, HttpVersion ver) {
    AddVersion(ver);
    AddStatusCodeAndString(code);
    return *this;
  }

  Self& AddHeaderLine(HttpStatusCode code) {
    return AddHeaderLine(code, HttpVersion::kHttp10);    
  }

  /**
   * Add header in response line
   * including its field and content.
   * @param field field name
   * @param content corresponding description
   */
  Self& AddHeader(kanon::StringArg field,
                  kanon::StringArg content) {
    char buf[256];

    ::snprintf(buf, sizeof buf, "%s: %s\r\n", field.data(), content.data());

    buffer_.Append(buf);
    return *this;
  }

  /**
   * Add blankline after header lines.
   * It should be \r\n\r\n(other may be invalid to browser)
   */
  Self& AddBlackLine() {
    buffer_.Append("\r\n");
    return *this;
  }

  Self& AddHeaderBlackLine() {
    return *this;
  }

  /**
   * Add body contents.
   * @param content body content
   */
  Self& AddBody(kanon::StringView content) {
    body_.Append(content);
    return *this;
  }

  template<size_t N>
  Self& AddBody(char const(&buf)[N]) {
    return AddBody(kanon::MakeStringView(buf));
  }

  template<typename ...Args>
  Self& AddBody(char* buf, size_t len, kanon::StringArg fmt, Args... args) {
    body_.Append(kanon::StringView(buf, ::snprintf(buf, len, fmt.data(), args...)));
    return *this;
  }

  template<unsigned N, typename ...Args>
  Self& AddBody(char(&buf)[N], kanon::StringArg fmt, Args... args) {
    return AddBody(buf, N, fmt, args...);
  }

  kanon::Buffer& GetBuffer() noexcept
  {
    char buf[128];
    MemoryZero(buf);
    ::snprintf(buf, sizeof buf, "%lu", body_.GetReadableSize());
    AddHeader("Content-Length", buf);
    AddBlackLine();
    buffer_.Append(body_.ToStringView());
    return buffer_;
  }
  
private:
  /**
   * Add version code in response line.
   * Only support HTTP0.0 and HTTP1.1.
   * @param ver valid http version code
   */
  Self& AddVersion(HttpVersion ver) {
    buffer_.Append(GetHttpVersionString(ver));
    return *this;
  }

  /**
   * Add status code in response line
   * and corresponding description string.
   * @param code valid status code
   */ 
  Self& AddStatusCodeAndString(HttpStatusCode code) {
    char buf[255];
    snprintf(buf, sizeof buf, " %d %s\r\n",
        GetStatusCode(code), GetStatusCodeString(code));
    
    buffer_.Append(static_cast<char const*>(buf));
    return *this;
  }

  // Temp buffer
  // It should be passed to Send(Buffer&)
  kanon::Buffer buffer_;
  kanon::Buffer body_;
};

HttpResponse GetClientError(HttpStatusCode status_code,
                            kanon::StringView msg);

} // namespace http

#endif // KANON_HTTP_RESPONSE_H