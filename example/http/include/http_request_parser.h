#ifndef KANON_HTTP_REQUEST_PARSER_H
#define KANON_HTTP_REQUEST_PARSER_H

#include "kanon/util/noncopyable.h"
#include "kanon/string/string_view.h"
#include "kanon/net/buffer.h"
#include "kanon/net/callback.h"

#include "http_constant.h"
#include "kanon/util/optional.h"
#include "types.h"

namespace http {

enum class ParseResult {
  kComplete = 0, // complete line
  kShort, // short red
  kBad,  // bad line
};

  
/**
 * Parse request content, which include request line and header line
 * @note noncopyable
 */
class HttpRequestParser : kanon::noncopyable {
  enum class ParseState {
    kRequestLine = 0,
    kHeader,
    kBody,
    kComplete
  };

  // used for parse of request line 
  enum class RequestLineState {
    kStart = 0,
    kMethod,
    kSpacesBeforeUrl,
    kScheme,
    kSchemeSlash,
    kSchemeDoubleSlash,
    kHost,
    kPort,
    kAfterSlashInUrl,
    kCheckUrl,
    kUrl,
    kH,
    kHT,
    kHTT,
    kHTTP,
    kFirstMajorDigit,
    kMajorDigit,
    kVersionDot,
    kFirstMinorDigit,
    kMinorDigit,
    kCRLFAfterMinor,
    kAlmostDone,
    kDone,
    kNum,
  };

  enum class ComplexUrlState {
    kUsual = 0,
    kQuery,
    kSlash,
    kDot,
    kDoubleDot,
    kPersentFirst,
    kPersentSecond,
  };

  enum class ParseRequestLineResult {
    kOk = 0, // coming a black line
    kAgain,
    kInvalidMethod,
    kInvalidUrl,
    kInvalidHttpVersion,
    kInvalidHttpRequest
  };
public:
  HttpRequestParser();

  /**
   * Parse the http reqeust in @p buffer
   */ 
  ParseResult Parse(kanon::Buffer& buffer);

  bool IsStatic() const noexcept { return is_static_; }

  HttpMethod GetMethod() const noexcept { return method_; }
  HeaderMap const& GetHeaders() const noexcept { return headers_; }
  kanon::optional<kanon::StringView> GetHeaderValue(std::string field) const noexcept;

  std::string const& GetUrl() const noexcept { return url_; }
  kanon::StringView GetErrorString() const noexcept { return error_string_; }
  HttpVersion GetHttpVersion() const noexcept;
  HttpStatusCode GetErrorStatusCode() const noexcept { return error_status_code_; }  
  size_t GetCacheContentLength() const noexcept { return cache_content_length_; }

  void Reset() noexcept { new (this) HttpRequestParser(); }

  ParseResult ProcessRequestLine(kanon::StringView request_line);
  ParseResult ProcessRequestHeader(kanon::StringView headers);

  bool ParseURLFormat(kanon::StringView body, std::string& query);
  
private:
  /**
   * Parse the request header line
   * @see RFC 2616
   */
  ParseRequestLineResult ParseRequestLine(kanon::StringView line);

  ParseRequestLineResult ParseComplexUrl();

  void ParseMethod(kanon::StringView method) noexcept;

  void FillErrorString(kanon::StringArg str) noexcept;
  void SetErrorStatusCodeOfRequestLine(ParseRequestLineResult result) noexcept;

  // For debugging
  void PrintRequestContent(
    kanon::StringView msg,
    kanon::Buffer const& buffer);

  // char const* request_start_ = nullptr;
  // char const* request_end_ ;
  char const* scheme_start_ ;
  char const* scheme_end_ ;
  char const* host_start_ ;
  char const* host_end_ ;
  char const* port_start_ ;
  char const* port_end_ ;
  char const* url_start_ ;
  char const* url_end_ ;

  // /** Args start with '?' */
  // char const* args_start_ = nullptr;

  /** Current parsed position */
  char const* parsed_pos_    = nullptr;
  //char const* ext_start_     = nullptr;
  //char const* argsEnd_ = nullptr;
  //char const* extEnd_ = nullptr;
  
  int http_major_;
  int http_minor_;

  // int http_version_;

  // std::string path_;
  // std::string args_;

  std::string url_;
  // Use int instead specific enum 
  // to reuse state in different request
  // phase.
  // e.g. RequestLineState and ComplexUrlState
  int state_; 
  
  ParseState phase_;
  HttpMethod method_;

  /**
   * complex_url_ is not equivalent to is_static_.
   * For example, %20 is complex url but may be static also.  
   * i.e. complex url contains the static case
   */
  bool complex_url_;
  bool is_static_;
  HeaderMap headers_;

  size_t cache_content_length_ = 0;
  std::string error_string_;
  HttpStatusCode error_status_code_ = kInvalidStatusCode;
  // For debugging only
  static char const* request_line_state_strings[(unsigned long)RequestLineState::kNum];
};

} // namespace http

#endif // KANON_HTTP_REQUEST_PARSER_H