#include "HttpRequestParser.h"
#include "HttpMacro.h"

#include "HttpResponse.h"
#include "kanon/net/TcpConnection.h"

IMPORT_NAMESPACE(kanon);
IMPORT_NAMESPACE(http);
IMPORT_NAMESPACE(std);

#define SERVER_PATH "/root/kanon/example/http"

static inline bool 
inRange(char c, char b, char e) {
  return c >= b && c <= e;
}

ParseResult
HttpRequestParser::parse(kanon::TcpConnectionPtr const& conn, Buffer& buffer) {
  if (!parsedPos_) {
    parsedPos_ = buffer.peek();
  }

  while (buffer.readable_size() > 0 && phase_ < RequestPhase::kComplete) {
    switch (phase_) {
    case RequestPhase::kRequestLine:
      LOG_TRACE << "Parse RequestLine";
      switch (processRequestLine(conn)) {
      case ParseResult::kShort:
        return ParseResult::kShort;

      case ParseResult::kComplete:
        //phase_ = RequestPhase::kHeader;
        phase_ = RequestPhase::kComplete;
        buffer.advance(parsedPos_-buffer.peek());
        break;
      case ParseResult::kBad:
        LOG_DEBUG << "Accept a bad request or internal error";
        return ParseResult::kBad;
        break;
      default:
        LOG_ERROR << "Not desirable parse result";
        return ParseResult::kBad;
        break;
      }
      break;

    case RequestPhase::kHeader:
      // not implemeted
    case RequestPhase::kComplete:
      return ParseResult::kComplete;
      break;
    default:
      return ParseResult::kBad;
      break;
    } // end which (phase_)
  } // end while
   
  if (phase_ == RequestPhase::kComplete) {
    return ParseResult::kComplete;
  } 

  return ParseResult::kBad;
}

// For debugging
char const*
HttpRequestParser::requestLineStateStrings[] = {
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
  "FirstMinorDigit",
  "MinorDigit",
  "AlmostDone",
  "Done",
};

ParseRequestLineStatus
HttpRequestParser::parseRequestLine(StringView line) {
  // RequestLine format:
  // Method (space_after_method) (spaces before url) URL (linear spaces) HttpVersion
  RequestLineState state = (RequestLineState)state_;
   
  for (; parsedPos_ != line.end() 
      && state < RequestLineState::kDone; ++parsedPos_) {
    char c = *parsedPos_;;
    
    LOG_DEBUG << "state: " << requestLineStateStrings[(int)state];

    switch (state) {
    case RequestLineState::kStart:
      // @see RFC 2616 5.1 Request-Line
      // The elements are separated by SP characters. No CR or LF is allowed except in the final CRLF sequence.
      if (c == CR|| c == LF)
        return ParseRequestLineStatus::kInvalidHttpRequest;
      // @see RFC 2616 5.1.1 Method
      // The method is case-sensitive.
      // Therefore, if c is not upper case letter, it is invalid
      if (!inRange(c, 'A', 'Z')) {
        return ParseRequestLineStatus::kInvalidMethod;
      }
      
      requestStart_ = parsedPos_; 

      state = RequestLineState::kMethod;
      break; 

    case RequestLineState::kMethod:
      if (c == ' ') {
        // Method has extracted
        auto method = line.substr(
          requestStart_-line.begin(),
          parsedPos_-requestStart_);
        
        LOG_DEBUG << "method: " << method;

        parseMethod(method);

        if (method_ == HttpMethod::kNotSupport) {
          return ParseRequestLineStatus::kInvalidMethod;
        }

        state = RequestLineState::kSpacesBeforeUrl;

        break;
      }

      if (!inRange(c, 'A', 'Z'))
        return ParseRequestLineStatus::kInvalidMethod;
    
      break;

    case RequestLineState::kSpacesBeforeUrl:
      switch (c) {
      // omit these spaces before Url
      case ' ':
        break;
      // no scheme:
      // /path-content
      case '/':
        urlStart_ = parsedPos_;
        state = RequestLineState::kAfterSlachInUrl;
        break;
      default:
        // scheme:
        // http/ftp/...://path-content
        //
        // RFC 3986 6.2.2.1 Case Normalization
        // scheme and host are case-insensitive and therefore
        // should be normalized to lowercase
        if (inRange(c, 'a', 'z') || inRange(c, 'A', 'Z')) {
          state = RequestLineState::kScheme;
          schemeStart_ = parsedPos_;
          break;
        }

        return ParseRequestLineStatus::kInvalidUrl;
      } 

      break;

    case RequestLineState::kScheme:
      switch (c) {
      // scheme:
      case ':':
        schemeEnd_ = parsedPos_;
        state = RequestLineState::kSchemeSlash;
        break;
      default:
        if (!inRange(c, 'a', 'z') && !inRange(c, 'A', 'Z'))
          return ParseRequestLineStatus::kInvalidUrl;

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
        return ParseRequestLineStatus::kInvalidUrl;
      }

      break;

    case RequestLineState::kSchemeDoubleSlash:
      // scheme://
      switch (c) {
      case '/':
        hostStart_ = parsedPos_ + 1;
        state = RequestLineState::kHost;
        break;
      default:
        return ParseRequestLineStatus::kInvalidUrl;
      }
      
      break;

    case RequestLineState::kHost:
      // sheme://host
      switch (c) {
      case '/':
        // Default port
        hostEnd_ = parsedPos_;
        urlStart_ = parsedPos_;
        state = RequestLineState::kAfterSlachInUrl;
        break;
      case ':':
        hostEnd_ = parsedPos_;
        portStart_ = parsedPos_ + 1;
        state = RequestLineState::kPort;
        break;
      default:
        // FIXME see RFC 3986 3.2.2 host
        if (inRange(c, 'a', 'z') || inRange(c, 'A', 'Z')
            || c == '.' || c == '-' || inRange(c, '0', '9')) {
          break;
        }

        return ParseRequestLineStatus::kInvalidUrl;
      }

      break;

    case RequestLineState::kPort:
      switch (c) {
      case '/':
        // If portStart_ == portEnd_,
        // /host:/ ==> default port
        portEnd_ = parsedPos_;
        urlStart_ = parsedPos_;
        state = RequestLineState::kAfterSlachInUrl;
        break;
      default:
        if (!inRange(c, '0', '9'))
          return ParseRequestLineStatus::kInvalidUrl;

        break;
      }

      break;
    case RequestLineState::kAfterSlachInUrl:
      switch (c) {
      case '?':
        argsStart_ = parsedPos_ + 1;
        isStatic_ = false;
        complexUrl_ = true;
        state = RequestLineState::kUrl;
        break;
      // '.' used to identify relative path
      case '.':
      case '%':
        complexUrl_ = true;
        state = RequestLineState::kUrl;
        break;
      case '/':
        break;
      // space before http version
      case ' ':
        urlEnd_ = parsedPos_;
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
        extStart_ = nullptr;
        state = RequestLineState::kAfterSlachInUrl;
        break;
      case '?':
        argsStart_ = parsedPos_ + 1;
        complexUrl_ = true;
        isStatic_ = false;
        state = RequestLineState::kUrl;
        break;
      case '%':
        complexUrl_ = true;
        state = RequestLineState::kUrl;
        break;
      // '.' used to indicate extension type
      case '.':
        extStart_ = parsedPos_ + 1;
        break;
      case ' ':
        urlEnd_ = parsedPos_;
        state = RequestLineState::kH;
        break;
      default:
        break;
      }

      break;

    case RequestLineState::kUrl:
      switch (c) {
      case ' ':
        urlEnd_ = parsedPos_;
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
          return ParseRequestLineStatus::kInvalidHttpVersion;
        }
        
        state = RequestLineState::kHT;
        break;
      }

      break;

    case RequestLineState::kHT:
      if (c != 'T') {
        return ParseRequestLineStatus::kInvalidHttpVersion;
      }

      state = RequestLineState::kHTT;

      break;
    
    case RequestLineState::kHTT:
      if (c != 'T') {
        return ParseRequestLineStatus::kInvalidHttpVersion;
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
          return ParseRequestLineStatus::kInvalidHttpVersion;
        }
      } 

      break;
    
    case RequestLineState::kFirstMajorDigit:
      if (!inRange(c, '0', '9')) {
        return ParseRequestLineStatus::kInvalidHttpVersion;
      }

      httpMajor_ = c - '0';
      state = RequestLineState::kMajorDigit;
    
      break;

    case RequestLineState::kMajorDigit:
      switch (c) {
        case '.':
          state = RequestLineState::kFirstMinorDigit;
          break;
        default:
          if (!inRange(c, '0', '9')) {
            return ParseRequestLineStatus::kInvalidHttpVersion;
          }
          
          httpMajor_ = httpMajor_ * 10 + c - '0';
          state = RequestLineState::kFirstMinorDigit;
          break;
      }

      break;

    case RequestLineState::kFirstMinorDigit:
      if (!inRange(c, '0', '9')) {
        return ParseRequestLineStatus::kInvalidHttpVersion;
      }

      httpMinor_ = c - '0';
      state = RequestLineState::kMinorDigit;
      break;

    case RequestLineState::kMinorDigit:
      switch (c) {
      case CR:
        state = RequestLineState::kAlmostDone;
        break;

      // If only one LF, also accept it.
      case LF:
        requestEnd_ = parsedPos_ - 1;
        state = RequestLineState::kDone;
        break;
      default:
        if (!inRange(c, '0', '9')) {
          return ParseRequestLineStatus::kInvalidHttpVersion;
        }

        httpMinor_ = httpMinor_ * 10 + c - '0';
        break;
      }

      break;

      case RequestLineState::kAlmostDone:
        requestEnd_ = parsedPos_ - 1;

        if (c != LF) {
          return ParseRequestLineStatus::kInvalidHttpRequest; 
        }

        state = RequestLineState::kDone;
        break;

      case RequestLineState::kDone:
        break;
    } // end switch (state)
  } // end for 
    
  if (state == RequestLineState::kDone) {
    httpVersion_ = httpMajor_ * 1000 + httpMinor_;
    state_ = (int)RequestLineState::kStart;
    // FIXME reset parsedPos_ and other(if need)
    return ParseRequestLineStatus::kOk;
  } else {
    return ParseRequestLineStatus::kAgain;
  }
}

ParseRequestLineStatus
HttpRequestParser::parseComplexUrl() {
  auto p = urlStart_;
  
  path_.reserve(urlEnd_-urlStart_+1);
  path_ = "";  
  
  // Normally, urlEnd_ is the end of args
  // if args exists actually.
  if (argsStart_) { 
    args_.reserve(urlEnd_ - argsStart_);
    args_ = "";
  }

  auto state = (ComplexUrlState)state_;
  auto persent_trap = state; // trap persent state to back previous state
  
  unsigned char decode_persent = 0; // Hexadecimal digit * 2 after %

  if (urlStart_ && *urlStart_ != '/')
    return ParseRequestLineStatus::kInvalidUrl;
  
  //path_ = '.';
  path_ = SERVER_PATH;

  for (; p < urlEnd_; ++p) {
    char c = *p;
    
    switch (state) {
      case ComplexUrlState::kUsual:
        switch (c) {
        case '/':
          state = ComplexUrlState::kSlash;
          path_ += c;
          break;
        case '%':
          persent_trap = state;
          state = ComplexUrlState::kPersentFirst;
          break;
        case '?':
          state = ComplexUrlState::kQuery;
          break;
        default:
          path_ += c;
          break;
        }
        
        break;
      
      case ComplexUrlState::kQuery:
        switch (c) {
        case '/':
          state = ComplexUrlState::kSlash;
          args_ += c;
          break;
        case '%':
          persent_trap = state;
          state = ComplexUrlState::kPersentFirst;
          break;
        case '?':
          state = ComplexUrlState::kQuery;
          break;
        default:
          args_ += c;
          break;
        } 

        break;

      case ComplexUrlState::kSlash:
        switch (c) {
        case '.':
          state = ComplexUrlState::kDot;
          break;
        case '%':
          persent_trap = state;
          state = ComplexUrlState::kPersentFirst;
          break;
        default:
          break;
        }
        break;

      case ComplexUrlState::kDot:
        switch (c) {
        case '/':
          // /./
          state = ComplexUrlState::kSlash;
          break;
        case '.':
          state = ComplexUrlState::kDoubleDot;
          break;
        default:
          path_ += c;
          state = ComplexUrlState::kUsual;
          break;
        }
        break;

      case ComplexUrlState::kDoubleDot:
        switch (c) {
        // A/B/../ ==> A/
        case '/':
          while (path_.back() != '/') {
            path_.pop_back();
          }

          state = ComplexUrlState::kSlash;
          break;
        case '%':
          persent_trap = state;
          state = ComplexUrlState::kPersentFirst;
          break; 

        default:
          state = ComplexUrlState::kUsual;
          path_ += c;
          break;
        }
        break;
      
      case ComplexUrlState::kPersentFirst:
        if (inRange(c, '0', '9')) {
          decode_persent = c - '0';
          state = ComplexUrlState::kPersentSecond;
          break;
        }
        
        c = c | 0x20; // to lower case letter

        if (inRange(c, 'a', 'f')) {
          decode_persent = c - 'a' + 10;
          state = ComplexUrlState::kPersentSecond;
          break;
        }
        
        return ParseRequestLineStatus::kInvalidUrl;
        break; 

      case ComplexUrlState::kPersentSecond:
        if (inRange(c, '0', '9')) {
          decode_persent = (decode_persent << 4) + c - '0';
          state = persent_trap;
          path_ += (char)decode_persent;
          break;
        }
        
        c = c | 0x20;

        if (inRange(c, 'a', 'f')) {
          decode_persent = (decode_persent << 4) + c - 'a' + 10;
          state = persent_trap;
          path_ += (char)decode_persent;
          break;
        }
        return ParseRequestLineStatus::kInvalidUrl;
    } // end switch (state)
  } // end for
  
  return ParseRequestLineStatus::kOk;
}

ParseResult 
HttpRequestParser::processRequestLine(TcpConnectionPtr const& conn) {
  switch (parseRequestLine(conn->inputBuffer()->toStringView())) {
  case ParseRequestLineStatus::kOk:
    if (complexUrl_) {
      switch (parseComplexUrl()) {
      case ParseRequestLineStatus::kOk:
        return ParseResult::kComplete;
        break;
      case ParseRequestLineStatus::kInvalidUrl:
        conn->send(clientError(HttpStatusCode::k400BadRequest, "Invalid URL format").buffer()); 
        break;
      default:
        conn->send(clientError(HttpStatusCode::k500InternalServerError, "Unknown internel server error").buffer());
        break;
      }
    } else {
      // set file path
      
      path_.reserve(sizeof SERVER_PATH+urlEnd_-urlStart_); 
      //path_ = '.';
      path_ = SERVER_PATH;
      path_ += StringView{ urlStart_, static_cast<StringView::size_type>(urlEnd_-urlStart_) }.toString();
    }

    if (path_.back() == '/') {
      char const defaultPage[] = "index.html";
      path_.reserve(path_.size() + sizeof defaultPage);
      path_ += defaultPage;
    }

    LOG_DEBUG << "File Path: " << path_;
  
    LOG_DEBUG << "processRequestLine OK";
    return ParseResult::kComplete;
    break; 
  case ParseRequestLineStatus::kAgain:
    return ParseResult::kShort;
  case ParseRequestLineStatus::kInvalidMethod:
    conn->send(clientError(HttpStatusCode::k405MethodNotAllowd, "Invalid method format").buffer());
    break;
  case ParseRequestLineStatus::kInvalidUrl:
    conn->send(clientError(HttpStatusCode::k400BadRequest, "Invalid URL format").buffer()); 
    break;
  case ParseRequestLineStatus::kInvalidHttpVersion:
    conn->send(clientError(HttpStatusCode::k400BadRequest, "Invalid http version in request").buffer());
    break;
  default:
    conn->send(clientError(HttpStatusCode::k500InternalServerError, "Unknown internel server error").buffer());
    break;
  }
  
  conn->shutdownWrite();
  return ParseResult::kBad;
}
void
HttpRequestParser::parseMethod(StringView method) noexcept {
  if (!method.compare("GET")) {
    method_ = HttpMethod::kGet;
  } else if (!method.compare("POST")) {
    method_ = HttpMethod::kPost;
  } else if (!method.compare("PUT")) {
    method_ = HttpMethod::kPut;
  } else if (!method.compare("HEAD")) {
    method_ = HttpMethod::kHead;
  } else {
    method_ = HttpMethod::kNotSupport;
  }

}