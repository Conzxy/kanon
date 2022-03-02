#ifndef KANON_HTTPSERVER_H
#define KANON_HTTPSERVER_H

#include <unordered_map>

#include "http_response.h"
#include "kanon/net/callback.h"
#include "kanon/net/user_server.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/string/string_view.h"

#include "file.h"
#include "http_constant.h"
#include "http_request_parser.h"

namespace http {

class HttpServer : public kanon::TcpServer {
  // In fact, use raw pointer is also ok
  // But in some case, it is not good.
  using FilePtr = std::shared_ptr<File>;
public:
  // Default port is 80
  explicit HttpServer(EventLoop* loop)
    : HttpServer(loop, InetAddr(80))
  { 
  }

  HttpServer(EventLoop* loop, InetAddr const& listen_addr);

private:
  void OnMessage(TcpConnectionPtr const& conn, Buffer& buffer, TimeStamp recv);

  void OnConnection(TcpConnectionPtr const& conn);

  void SendFile(
    TcpConnectionPtr const& conn,
    FilePtr const& file,
    HttpRequestParser const& parser);

  void ExecuteCgi(
    TcpConnectionPtr const& conn,
    HttpRequestParser const& parser);
    

  void ServeFile(
    TcpConnectionPtr const& conn,
    HttpRequestParser const& parser);

  static void LastWriteComplete(
    kanon::TcpConnectionPtr const& conn,
    HttpRequestParser const& parser);

  static void CloseConnection(
    kanon::TcpConnectionPtr const& conn,
    HttpRequestParser const& parser);

  static void SendErrorResponse(
    kanon::TcpConnectionPtr const& conn,
    HttpStatusCode code,
    kanon::StringView msg,
    HttpRequestParser const& parser);

  static void SendErrorResponse(
    kanon::TcpConnectionPtr const& conn,
    HttpStatusCode code,
    HttpRequestParser const& parser)
  { SendErrorResponse(conn, code, kanon::MakeStringView(""), parser); }

  static char const kHomePage_[];
  static char const kRootPath_[];
  static char const kHtmlPath_[];
  static char const kHost_[];
  static constexpr int32_t kFileBufferSize_ = 1 << 16;
};
}

#endif // KANON_HTTPSERVER_H
