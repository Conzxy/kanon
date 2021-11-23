#ifndef KANON_HTTP_REQUEST_PARSER_H
#define KANON_HTTP_REQUEST_PARSER_H

#include "HttpGlobal.h"
#include "types.h"

#include "kanon/util/noncopyable.h"
#include "kanon/string/string_view.h"
#include "kanon/net/Buffer.h"

#include "kanon/net/callback.h"

namespace http {


enum class ParseResult {
  kComplete = 0, // complete line
  kShort, // short red
  kBad,  // bad line
};

enum class ParseRequestLineStatus {
  kOk = 0, // coming a black line
  kAgain,
  kInvalidMethod,
  kInvalidUrl,
  kInvalidHttpVersion,
  kInvalidHttpRequest
};
  
/**
 * @class HttpRequestParser
 * @brief 
 * Parse request content, which include request line and header line
 * @note noncopyable
 */
class HttpRequestParser : kanon::noncopyable {
  enum class RequestPhase {
    kStart = 0,
    kRequestLine,
    kHeader,
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
    kAfterSlachInUrl,
    kCheckUrl,
    kUrl,
    kH,
    kHT,
    kHTT,
    kHTTP,
    kFirstMajorDigit,
    kMajorDigit,
    kFirstMinorDigit,
    kMinorDigit,
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

public:
  HttpRequestParser()
    : complexUrl_{ false }
    , state_{ 0 }
    , phase_{ RequestPhase::kRequestLine }
    , method_{ HttpMethod::kNotSupport }
    , isStatic_{ false }
  {
  }

  ParseResult parse(kanon::TcpConnectionPtr const& conn, 
      kanon::Buffer& buffer);

  ParseRequestLineStatus parseComplexUrl();

  ParseRequestLineStatus parseRequestLine(kanon::StringView line);

  bool isStatic() const noexcept
  { return isStatic_; }

  HttpMethod method() const noexcept
  { return method_; }
  
  std::string& path() noexcept
  { return path_; }

  std::string& args() noexcept
  { return args_; }
private:
  ParseResult processRequestLine(kanon::TcpConnectionPtr const& conn);
  void parseMethod(kanon::StringView method) noexcept;
  
  static char const* requestLineStateStrings[(unsigned long)RequestLineState::kNum];

  char const* requestStart_ = nullptr;
  char const* requestEnd_ = nullptr;
  char const* schemeStart_ = nullptr;
  char const* schemeEnd_ = nullptr;
  char const* hostStart_ = nullptr;
  char const* hostEnd_ = nullptr;
  char const* portStart_ = nullptr;
  char const* portEnd_ = nullptr;
  char const* urlStart_ = nullptr;
  char const* urlEnd_ = nullptr;
  char const* argsStart_ = nullptr;
  //char const* argsEnd_ = nullptr;
  char const* parsedPos_ = nullptr;
  char const* extStart_ = nullptr;
  //char const* extEnd_ = nullptr;

  bool complexUrl_;
  
  int httpMajor_;
  int httpMinor_;

  int httpVersion_;
  std::string path_;
  std::string args_;

  // Use int instead specific enum 
  // to reuse state in different request
  // phase.
  int state_; 
  
  RequestPhase phase_;
  HttpMethod method_;
  
  bool isStatic_;
  HeaderMap headers_;

};

} // namespace http

#endif // KANON_HTTP_REQUEST_PARSER_H
