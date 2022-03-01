#include "http_request_parser.h"
#include "http_constant.h"
#include "kanon/string/string_view.h"
#include "kanon/util/optional.h"
#include "macro.h"
#include "http_response.h"

#include "kanon/net/tcp_connection.h"

using namespace kanon;
using namespace std;

namespace http {

// FIXME Set this in config file
// char const kRootPath_[] = "/root/kanon/example/http";

// Helper
static inline bool 
InRange(char c, char b, char e) noexcept
{
  return c >= b && c <= e;
}

static inline bool
IsUpperLetter(char c) noexcept 
{
  return InRange(c, 'A', 'Z');
}

static inline bool
IsLowerLetter(char c) noexcept
{
  return InRange(c, 'a', 'z');
}

static inline bool
IsLetter(char c) noexcept
{
  return IsLowerLetter(c) || IsUpperLetter(c);
}

static inline bool
IsDigit(char c) noexcept 
{
  return InRange(c, '0', '9');
}

HttpRequestParser::HttpRequestParser()
  // : request_end_(nullptr)
  : scheme_start_(nullptr)
  , scheme_end_(nullptr)
  , host_start_(nullptr)
  , host_end_(nullptr)
  , port_start_(nullptr)
  , port_end_(nullptr)
  , url_start_(nullptr)
  , url_end_(nullptr)
  // , args_start_(nullptr)
  , parsed_pos_(nullptr)
  , http_major_(0)
  , http_minor_(0)
  // , http_version_(0)
  // , path_()
  // , args_()
  , url_()
  , state_(0)
  , phase_(ParseState::kRequestLine)
  , method_(HttpMethod::kNotSupport)
  , complex_url_(false)
  , is_static_(true)
  , headers_()
  , error_string_()
  , error_status_code_(kInvalidStatusCode)
{
}

// This is the main state machine
// ProcessRequestLine() is the sub state machine to parse the request header line
ParseResult HttpRequestParser::Parse(Buffer& buffer)
{
  KANON_ASSERT(buffer.GetReadableSize() != 0, "The buffer to parse must not be empty");

  if (parsed_pos_ == nullptr) {
    parsed_pos_ = buffer.GetReadBegin();
  }

  while (phase_ <= ParseState::kComplete) {
    switch (phase_) {

    // First, parse the request header line
    case ParseState::kRequestLine:
    {
      LOG_TRACE << "Parse requestLine Phase";

      const auto result = ProcessRequestLine(buffer.ToStringView());

      PrintRequestContent("Before advance of request line", buffer);

      if (result == ParseResult::kComplete) {
        LOG_DEBUG << "Request line: " 
          << buffer.ToStringView().substr(0, parsed_pos_-buffer.GetReadBegin());

        phase_ = ParseState::kHeader;
        
        buffer.AdvanceRead(parsed_pos_-buffer.GetReadBegin());

        PrintRequestContent("After advance of request line", buffer);
      }
      else {
        return result;
      }
    }
      break;

    case ParseState::kHeader:
    {
      LOG_TRACE << "Parse request Header";
      
      const auto result = ProcessRequestHeader(buffer.ToStringView());
      if (result == ParseResult::kComplete)
      {
        phase_ = ParseState::kBody;
        buffer.AdvanceRead(parsed_pos_-buffer.GetReadBegin());
      }
      else {
        return result;
      }
    }
      break;

    case ParseState::kBody:
      phase_ = ParseState::kComplete;
      break;
    case ParseState::kComplete:
      // Reset
      return ParseResult::kComplete;
      break;
    default:
      error_status_code_ = HttpStatusCode::k500InternalServerError;
      FillErrorString("Unknown internal server error");
      break;
    } // End which (phase_)
  } // End while

  return ParseResult::kBad;
}

ParseResult HttpRequestParser::ProcessRequestLine(StringView request_line) {
  switch (auto result = ParseRequestLine(request_line)) {
  case ParseRequestLineResult::kOk:
    if (complex_url_) {
      switch (ParseComplexUrl()) {
      case ParseRequestLineResult::kOk:
        return ParseResult::kComplete;
        break;
      default:
        SetErrorStatusCodeOfRequestLine(result);
      }
    }
    else {
      url_ = request_line.substr(url_start_-request_line.begin(), url_end_ - url_start_).ToString();
    }
  
    return ParseResult::kComplete;
    break; 
  case ParseRequestLineResult::kAgain:
    return ParseResult::kShort;
  default:
    SetErrorStatusCodeOfRequestLine(result);
  }
  
  return ParseResult::kBad;
}

HttpRequestParser::ParseRequestLineResult 
HttpRequestParser::ParseRequestLine(StringView line) {
  // RequestLine format:
  // Method (space_after_method) (spaces before url) URL (linear spaces) HttpVersion
  RequestLineState state = (RequestLineState)state_;

  if (parsed_pos_ == nullptr) {
    parsed_pos_ = line.begin();
  }

  for (; parsed_pos_ != line.end() && state < RequestLineState::kDone;
       ++parsed_pos_) {
    char c = *parsed_pos_;
    
    LOG_DEBUG << "State: " << request_line_state_strings[(int)state];

    switch (state) {
    case RequestLineState::kStart:
      // @see RFC 2616 5.1 Request-Line
      // The elements are separated by SP characters. No CR or LF is allowed except in the final CRLF sequence.
      if (c == CR || c == LF) {
        FillErrorString("Invalid request: Start with CR/LF");
        return ParseRequestLineResult::kInvalidHttpRequest;
      }
      // @see RFC 2616 5.1.1 Method
      // The method is case-sensitive.
      // Therefore, if c is not upper case letter, it is invalid
      if (!IsUpperLetter(c)) {
        FillErrorString("Invalid method: Method must contains upper case letter only");
        return ParseRequestLineResult::kInvalidMethod;
      }

      // FIXME Need? 
      // request_start_ = parsed_pos_; 

      state = RequestLineState::kMethod;
      break; 

    case RequestLineState::kMethod:
    {
      // Meeting the first space which after method
      // In fact, this is SpaceAfterMethod state, but I don't use goto and also no need do it
      if (c == ' ') {
        // Method has extracted
        // auto method = line.substr(
        //   request_start_-line.begin(),
        //   parsed_pos_-request_start_);

        const auto method = line.substr(0, parsed_pos_-line.begin());
        LOG_DEBUG << "Method: " << method;

        ParseMethod(method);


        if (method_ == HttpMethod::kNotSupport) {
          FillErrorString("Invalid method: Not supported");
          return ParseRequestLineResult::kInvalidMethod;
        }

        if (method_ == HttpMethod::kPost) {
          is_static_ = false;
        }

        state = RequestLineState::kSpacesBeforeUrl;

        break;
      }

      if (!IsUpperLetter(c)) {
        FillErrorString("Invalid method: Method must contains upper case letter only");
        return ParseRequestLineResult::kInvalidMethod;
      } 
    }
      break;
    case RequestLineState::kSpacesBeforeUrl:
      switch (c) {
      // Protocol allow there are many spaces, althouth one space is usual
      // Omit these spaces before Url
      case ' ':
        break;
      // No scheme:
      // /path-content
      case '/':
        url_start_ = parsed_pos_;
        state = RequestLineState::kAfterSlashInUrl;
        break;
      default:
        // Scheme:
        // http/ftp/...://path-content
        //
        // RFC 3986 6.2.2.1 Case Normalization
        // scheme and host are case-insensitive and therefore
        // should be normalized to lowercase
        if (IsLetter(c)) {
          state = RequestLineState::kScheme;
          scheme_start_ = parsed_pos_;
          break;
        }
        
        FillErrorString("Invalid URL: Scheme must be contains letter only");
        return ParseRequestLineResult::kInvalidUrl;
      } 

      break;

    case RequestLineState::kScheme:
      switch (c) {
      // scheme:
      case ':':
        scheme_end_ = parsed_pos_;
        state = RequestLineState::kSchemeSlash;
        break;
      default:
        if (!IsLetter(c)) {
          FillErrorString("Invalid URL: Scheme must be contains letter only");
          return ParseRequestLineResult::kInvalidUrl;
        }

        break;
      }

      break; 

    case RequestLineState::kSchemeSlash:
      // sheme:/
      switch (c) {
      case '/':
        state = RequestLineState::kSchemeDoubleSlash;
        break;
      default:
        FillErrorString("Invalid URL: The first slash of scheme missing");
        return ParseRequestLineResult::kInvalidUrl;
      }

      break;

    case RequestLineState::kSchemeDoubleSlash:
      // scheme://
      switch (c) {
      case '/':
        host_start_ = parsed_pos_ + 1;
        state = RequestLineState::kHost;
        break;
      default:
        FillErrorString("Invalid URL: The second slash of scheme missing");
        return ParseRequestLineResult::kInvalidUrl;
      }
      
      break;

    case RequestLineState::kHost:
      // sheme://host
      switch (c) {
      case '/':
        // Default port
        host_end_ = parsed_pos_;
        url_start_ = parsed_pos_;
        state = RequestLineState::kAfterSlashInUrl;
        break;
      case ':':
        host_end_ = parsed_pos_;
        port_start_ = parsed_pos_ + 1;
        state = RequestLineState::kPort;
        break;
      // FIXME Test
      // case ' ':
      //   state = RequestLineState::kH;
      //   break;
      default:
        // FIXME see RFC 3986 3.2.2 host
        if (IsLetter(c) || IsDigit(c) || c == '.' || c == '-' ) {
          break;
        }

        FillErrorString("Invalid URL: Host contains letter, digit, '.', and '-' only");
        return ParseRequestLineResult::kInvalidUrl;
      }

      break;

    case RequestLineState::kPort:
      switch (c) {
      case '/':
        // If port_start_ == port_end_,
        // /host:/ ==> default port
        port_end_ = parsed_pos_;
        url_start_ = parsed_pos_;
        state = RequestLineState::kAfterSlashInUrl;
        break;
      // case ' ':
      //   state = RequestLineState::kH;
      //   break;
      default:
        if (!IsDigit(c)) {
          FillErrorString("Invalid URL: Port contains digit only");
          return ParseRequestLineResult::kInvalidUrl;
        }

        break;
      }

      break;
    // Check //, /?, /., /.., /%
    case RequestLineState::kAfterSlashInUrl:
      switch (c) {
      case '?':
        // args_start_ = parsed_pos_;
        is_static_ = false;
        complex_url_ = true;
        state = RequestLineState::kUrl;
        break;
      // '.' used to identify relative path
      // /.. or /.
      case '.':
      case '%':
        complex_url_ = true;
        state = RequestLineState::kUrl;
        break;
      case '/':
        // Need to remove
        complex_url_ = true; 
        break;
      // space before http version
      case ' ':
        url_end_ = parsed_pos_;
        state = RequestLineState::kH;
        break;
      // To handle this case:
      // /../ or /./
      default:
        state = RequestLineState::kCheckUrl;
        break;
      }

      break;

    case RequestLineState::kCheckUrl:
      switch (c) {
      case '/':
        // ext_start_ = nullptr;
        state = RequestLineState::kAfterSlashInUrl;
        break;
      case '?':
        // args_start_ = parsed_pos_ + 1;
        complex_url_ = true;
        is_static_ = false;
        state = RequestLineState::kUrl;
        break;
      case '%':
        complex_url_ = true;
        state = RequestLineState::kUrl;
        break;
      // // '.' used to indicate extension type
      // case '.':
      //   ext_start_ = parsed_pos_ + 1;
      //   break;
      case ' ':
        url_end_ = parsed_pos_;
        state = RequestLineState::kH;
        break;
      default:
        break;
      }

      break;

    case RequestLineState::kUrl:
      switch (c) {
      case ' ':
        url_end_ = parsed_pos_;
        state = RequestLineState::kH;
        break;
      default:
        break;
      }
      break;

    case RequestLineState::kH:
      switch (c) {
      // Spaces before http version
      case ' ':
        break;
      default:
        if (c != 'H') {
          FillErrorString("Invalid Http Version: The H in HTTP is missing");
          return ParseRequestLineResult::kInvalidHttpVersion;
        }
        
        state = RequestLineState::kHT;
        break;
      }

      break;

    case RequestLineState::kHT:
      if (c != 'T') {
        FillErrorString("Invalid Http Version: The first T in HTTP is missing");
        return ParseRequestLineResult::kInvalidHttpVersion;
      }

      state = RequestLineState::kHTT;

      break;
    
    case RequestLineState::kHTT:
      if (c != 'T') {
        FillErrorString("Invalid Http Version: The second T in HTTP is missing");
        return ParseRequestLineResult::kInvalidHttpVersion;
      }

      state = RequestLineState::kHTTP;
      break;

    case RequestLineState::kHTTP:
      switch (c) {
      case '/':
        state = RequestLineState::kFirstMajorDigit;
        break;
      default:
        if (c != 'P') {
          FillErrorString("Invalid Http Version: The P in HTTP is missing");
          return ParseRequestLineResult::kInvalidHttpVersion;
        }
      } 

      break;

    /**
     * <major>.<minor>
     * Each may be incremented higher than a single digit, HTTP/2.4 is lower than HTTP/2.13
     * @see RFC 2616 HTTP Version
     */ 
    case RequestLineState::kFirstMajorDigit:
      if (!IsDigit(c)) {
        FillErrorString("Invalid Http Version: The first major digit is missing");
        return ParseRequestLineResult::kInvalidHttpVersion;
      }

      http_major_ = c - '0';
      state = RequestLineState::kMajorDigit;
    
      break;

    case RequestLineState::kMajorDigit:
      switch (c) {
        case '.':
          state = RequestLineState::kFirstMinorDigit;
          break;
        default:
          if (!IsDigit(c)) {
            FillErrorString("Invalid Http Version: The second major digit is missing");
            return ParseRequestLineResult::kInvalidHttpVersion;
          }
          
          http_major_ = http_major_ * 10 + c - '0';
          state = RequestLineState::kVersionDot;
          break;
      }

      break;
    
    case RequestLineState::kVersionDot:
      if (c != '.') {
        FillErrorString("Invalid Http Version: The dot between major and minor digit is missing");
        return ParseRequestLineResult::kInvalidHttpVersion;
      }

      state = RequestLineState::kFirstMinorDigit;
      break;

    case RequestLineState::kFirstMinorDigit:
      if (!IsDigit(c)) {
        FillErrorString("Invalid Http Version: The first minor digit is missing");
        return ParseRequestLineResult::kInvalidHttpVersion;
      }

      http_minor_ = c - '0';
      state = RequestLineState::kMinorDigit;
      break;

    case RequestLineState::kMinorDigit:
      switch (c) {
      case CR:
        state = RequestLineState::kAlmostDone;
        break;

      // If only one LF, also accept it.
      case LF:
        // request_end_ = parsed_pos_ - 1;
        state = RequestLineState::kDone;
        break;
      default:
        if (!IsDigit(c)) {
          FillErrorString("Invalid Http Version: The second minor digit is missing");
        }

        http_minor_ = http_minor_ * 10 + c - '0';
        state = RequestLineState::kCRLFAfterMinor;
      }

      break;
    
    case RequestLineState::kCRLFAfterMinor:
      switch (c) {
      case CR:
        state = RequestLineState::kAlmostDone;
        break;

      // If only one LF, also accept it.
      case LF:
        // request_end_ = parsed_pos_ - 1;
        state = RequestLineState::kDone;
        break;
      default:
        FillErrorString("Invalid Http request: The CR/LF after http version is missing");
        return ParseRequestLineResult::kInvalidHttpRequest;
      }
      break;

    case RequestLineState::kAlmostDone:
      // request_end_ = parsed_pos_ - 1;

      if (c != LF) {
        return ParseRequestLineResult::kInvalidHttpRequest; 
      }

      state = RequestLineState::kDone;
      break;
    } // end switch (state)
  } // end for 
    
  if (state == RequestLineState::kDone) {
    LOG_DEBUG << "State: Done";

    state_ = 0;
    return ParseRequestLineResult::kOk;
  } else {
    return ParseRequestLineResult::kAgain;
  }
}

HttpRequestParser::ParseRequestLineResult 
HttpRequestParser::ParseComplexUrl() {
  assert(complex_url_);
  assert(url_start_ && url_end_);  
  // assert(args_start_);
  assert(url_.empty());

  if (url_start_ && *url_start_ != '/')
    return ParseRequestLineResult::kInvalidUrl;

  return ParseURLFormat(StringView(url_start_, url_end_-url_start_), url_) ?
    ParseRequestLineResult::kOk : ParseRequestLineResult::kInvalidUrl;
}

ParseResult HttpRequestParser::ProcessRequestHeader(StringView headers)
{
  headers.remove_prefix(parsed_pos_ - headers.begin());

  StringView::size_type pos;

  if (headers.size() >= 2 && headers[0] == CR && headers[1] == LF) {
    parsed_pos_ += 2;
    return ParseResult::kComplete;
  }

  string field;
  string field_value;
  while ((pos = headers.find("\r\n")) != StringView::npos) {
    auto spliter = headers.find(':');

    if (spliter == StringView::npos) {
      FillErrorString("Header must have \":\"");
      error_status_code_ = HttpStatusCode::k400BadRequest;
      return ParseResult::kBad;
    }

    LOG_DEBUG << headers.substr(0, spliter) << ": " 
      << headers.substr(spliter+2, pos - spliter - 2);
    
    field = headers.substr(0, spliter).ToString();
    field_value = headers.substr(spliter+2, pos - spliter - 2).ToString();

    if (field == "Content-Length") {
      cache_content_length_ = atoll(field_value.c_str());
    }

    headers_.emplace(field, field_value);

    parsed_pos_ = headers.begin() + pos + 2;
    headers.remove_prefix(pos + 2);

    if (headers.size() >= 2 &&
        headers[0] == CR && 
        headers[1] == LF) {
      parsed_pos_ += 2;
      return ParseResult::kComplete;
    }
  }

  return ParseResult::kShort;
}

bool HttpRequestParser::ParseURLFormat(kanon::StringView body, std::string &query)
{
  assert(query.empty());

  query.reserve(body.size());

  auto state = (ComplexUrlState)state_;
  auto persent_trap = state; // trap persent state to back previous state
  
  unsigned char decode_persent = 0; // Hexadecimal digit * 2 after %, i.e. % Hex Hex

  for (auto c : body) {
    switch (state) {
    case ComplexUrlState::kUsual:
      switch (c) {
      case '/':
        state = ComplexUrlState::kSlash;
        query += c;
        break;
      case '%':
        persent_trap = state;
        state = ComplexUrlState::kPersentFirst;
        break;
      case '?':
        is_static_ = false;
        query += c;
        break;
      default:
        query += c;
        break;
      }
      
      break;
    
    case ComplexUrlState::kSlash:
      switch (c) {
      case '/':
        break;
      case '.':
        state = ComplexUrlState::kDot;
        break;
      case '%':
        persent_trap = state;
        state = ComplexUrlState::kPersentFirst;
        break;
      default:
        query += c;
        state = ComplexUrlState::kUsual;
        break;
      }
      break;

    case ComplexUrlState::kDot:
      switch (c) {
      case '/':
        // /./
        // Discard ./
        state = ComplexUrlState::kSlash;
        break;
      case '.':
        state = ComplexUrlState::kDoubleDot;
        break;
      case '%':
        persent_trap = state;
        state = ComplexUrlState::kPersentFirst;
        break;
      default:
        query += c;
        state = ComplexUrlState::kUsual;
        break;
      }
      break;

    case ComplexUrlState::kDoubleDot:
      switch (c) {
      // A/B/../ ==> A/
      case '/':
        query.erase(query.rfind('/', query.size() - 2) + 1);

        state = ComplexUrlState::kSlash;
        break;
      case '%':
        persent_trap = state;
        state = ComplexUrlState::kPersentFirst;
        break; 

      default:
        state = ComplexUrlState::kUsual;
        query += c;
        break;
      }
      break;
    
    case ComplexUrlState::kPersentFirst:
      if (IsDigit(c)) {
        decode_persent = c - '0';
        state = ComplexUrlState::kPersentSecond;
        break;
      }
      
      c = c | 0x20; // to lower case letter

      if (IsDigit(c)) {
        decode_persent = c - 'a' + 10;
        state = ComplexUrlState::kPersentSecond;
        break;
      }

      FillErrorString("The first digit of persent-encoding is invalid");
      return false;
      break; 

    case ComplexUrlState::kPersentSecond:
      if (IsDigit(c)) {
        decode_persent = (decode_persent << 4) + c - '0';
        state = persent_trap;
        query += (char)decode_persent;
        break;
      }
      
      c = c | 0x20;

      if (IsDigit(c)) {
        decode_persent = (decode_persent << 4) + c - 'a' + 10;
        state = persent_trap;
        query += (char)decode_persent;
        break;
      }

      FillErrorString("The second digit of persent-encoding is invalid");
      return false;
    } // end switch (state)
  } // end for

  return true;
}

void HttpRequestParser::ParseMethod(StringView method) noexcept {
  if (!method.compare("GET")) {
    method_ = HttpMethod::kGet;
  } 
  else if (!method.compare("POST")) {
    method_ = HttpMethod::kPost;
  } 
  else if (!method.compare("PUT")) {
    method_ = HttpMethod::kPut;
  } 
  else if (!method.compare("HEAD")) {
    method_ = HttpMethod::kHead;
  } 
  else {
    method_ = HttpMethod::kNotSupport;
  }

}

void HttpRequestParser::FillErrorString(StringArg str) noexcept
{
  error_string_ = str.data();
}

HttpVersion HttpRequestParser::GetHttpVersion() const noexcept
{
  int version = http_major_ * 100 + http_minor_;

  switch (version) {
  case 101:
    return HttpVersion::kHttp11;
  case 100:
    return HttpVersion::kHttp10;
  default:
    return HttpVersion::kNotSupport;
  }
}

void HttpRequestParser::SetErrorStatusCodeOfRequestLine(ParseRequestLineResult result) noexcept
{
  switch (result) {
  case ParseRequestLineResult::kInvalidMethod:
    error_status_code_ = HttpStatusCode::k405MethodNotAllowd;
    break;
  case ParseRequestLineResult::kInvalidUrl:
    error_status_code_ = HttpStatusCode::k400BadRequest;
    break;
  case ParseRequestLineResult::kInvalidHttpVersion:
    error_status_code_ = HttpStatusCode::k400BadRequest;
    break;
  default:
    error_status_code_ = HttpStatusCode::k500InternalServerError;
    FillErrorString("Server internal error");
    break;
  }
}

kanon::optional<kanon::StringView>
HttpRequestParser::GetHeaderValue(std::string field) const noexcept
{
  auto it = headers_.find(field);

  if (it != headers_.end()) {
    return kanon::make_optional(StringView(it->second));
  }
  else {
    return kanon::make_null_optional<StringView>();
  }
}

// For debugging
void HttpRequestParser::PrintRequestContent(
  StringView msg,
  Buffer const& buffer)
{
  LOG_DEBUG << msg;
  for (auto c : buffer.ToStringView()) {
    if (c == CR) {
      printf("CR");
    }
    else if (c == LF) {
      printf("LF\n");
    }
    else {
      putc(c, stdout);
    }
  }
}

char const*
HttpRequestParser::request_line_state_strings[] = {
  "Start",
  "Method",
  "SpacesBeforeUrl",
  "Scheme",
  "SchemeSlash",
  "SchemeDoubleSlash",
  "Host",
  "Port",
  "AfterSlachInUrl",
  "CheckUrl",
  "Url",
  "H",
  "HT",
  "HTT",
  "HTTP",
  "FirstMajorDigit",
  "MajorDigit",
  "VersionDot",
  "FirstMinorDigit",
  "MinorDigit",
  "CRLFAfterMinor",
  "AlmostDone",
  "Done",
};

} // namespace http