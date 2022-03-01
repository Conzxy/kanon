#include <memory>
#include <unistd.h>

#include "kanon/log/logger.h"
#include "kanon/net/callback.h"
#include "kanon/string/string_view.h"
#include "kanon/util/unique_ptr.h"
#include "kanon/util/mem.h"

#include "file.h"
#include "http_constant.h"
#include "http_request_parser.h"
#include "http_server.h"
#include "macro.h"
#include "http_response.h"
#include "pipe.h"
#include "process.h"

using namespace kanon;
using namespace http;
using namespace std;
using namespace unix;

char const HttpServer::kRootPath_[] = "/root/kanon/example/http";
char const HttpServer::kHtmlPath_[] = "/root/kanon/example/http/html";
char const HttpServer::kHomePage_[] = "index.html";

HttpServer::HttpServer(EventLoop* loop, InetAddr const& listen_addr)
  : TcpServer(loop, listen_addr, "HttpServer")
{
  SetMessageCallback(std::bind(&HttpServer::OnMessage, this, _1, _2, _3));
  SetConnectionCallback(std::bind(&HttpServer::OnConnection, this, _1));
}

void HttpServer::OnMessage(
  TcpConnectionPtr const& conn,
  Buffer& buffer,
  TimeStamp recv)
{
  KANON_UNUSED(recv);
  auto p_parser = AnyCast<HttpRequestParser*>(conn->GetContext());
  assert(p_parser);

  auto parser = *p_parser;

  switch (parser->Parse(buffer)) {
    case ParseResult::kComplete:
    {
      switch (parser->GetMethod()) {
      case HttpMethod::kGet:
        if (parser->IsStatic()) {
          ServeFile(conn, *parser);
        }
        else {
          ExecuteCgi(conn, *parser);
        }
        break;
      case HttpMethod::kPut:
        break;
      case HttpMethod::kPost:
        ExecuteCgi(conn, *parser);
        break;
      case HttpMethod::kHead:
        break;
      case HttpMethod::kTrace:
        break;
      case HttpMethod::kOptions:
        break;
      case HttpMethod::kDelete:
        break;
      case HttpMethod::kNotSupport:
        LOG_ERROR << "Method is not support";
        break;
      default:
        break;
      }
    }
      break;
    case ParseResult::kBad:
      SendErrorResponse(conn, parser->GetErrorStatusCode(), parser->GetErrorString());

      LOG_DEBUG << "Parse Bad: " << parser->GetErrorString();
      break;
    case ParseResult::kShort:
      LOG_DEBUG << "Parse Short";
      break;
    default:
      KANON_ASSERT(false, "Server internal error");
  }


}

void HttpServer::ServeFile(
  TcpConnectionPtr const& conn,
  HttpRequestParser const& parser)
{
  std::string local_path;

  // FIXME 
  // Map path to different directory according to the extension type
  // e.g. *.jpg, *.png
  auto path = parser.GetUrl();

  if (path == "/") {
    path += kHomePage_;
  }

  local_path.reserve(path.size() + sizeof kRootPath_);
  local_path += kHtmlPath_;
  local_path += path;

  LOG_DEBUG << "local_path = " << local_path;
  LOG_DEBUG << "path = " << path;

  FilePtr file = new File();

  if (file->Open(local_path, File::kRead)) {
    conn->SetWriteCompleteCallback(
      [file, path, this](TcpConnectionPtr const& conn)
      {
        this->SendFile(conn, file);
      });

    SendFile(conn, file);
  }
  else {
    if (errno == ENOENT) {
      conn->Send(GetClientError(HttpStatusCode::k404NotFound, "File Not Found").GetBuffer());
    }
    else {
      LOG_SYSERROR << "Failed to open file: " << local_path;
    }
  }
}

void HttpServer::OnConnection(TcpConnectionPtr const& conn)
{
  DefaultConnectionCallback(conn);
  if (conn->IsConnected()) {
    conn->SetContext(new HttpRequestParser{});
  } else {
    delete *AnyCast<HttpRequestParser*>(conn->GetContext());
  }

}

void HttpServer::SendFile(
  TcpConnectionPtr const& conn,
  FilePtr file)
{
  char buf[kFileBufferSize];

  auto readn = file->Read(buf);

  if (readn != File::kInvalidReturn) {
    LOG_DEBUG << "readn = " << readn;
    if (readn > 0 && file->IsEof()) {
      assert(readn <= sizeof buf);

      LOG_DEBUG << "readn > 0 and meet EOF";
      conn->SetWriteCompleteCallback(
        [file](TcpConnectionPtr const& conn)
        {
          std::unique_ptr<File> guard(file);
          conn->ShutdownWrite();
        });

        conn->Send(StringView(buf, readn));
    }
    else if (readn > 0) { // readn > 0 && not EOF
      conn->Send(StringView(buf, readn));
    }
    else if (file->IsEof()) { // readn = 0 && EOF
      LOG_DEBUG << "readn = 0 and meet EOF";
      std::unique_ptr<File> guard(file);
      conn->ShutdownWrite();
      conn->SetWriteCompleteCallback(WriteCompleteCallback());
    }
  }
  else { // Error
    conn->SetWriteCompleteCallback(WriteCompleteCallback());
  }

}

void HttpServer::ExecuteCgi(
  TcpConnectionPtr const& conn,
  HttpRequestParser const& parser)
{
  LOG_TRACE << "Handle cgi request";

  // Child process(cgi) write to parent
  Pipe output;
  // Parent process write to child(cgi)
  Pipe input;

  Process process;

  conn->Send("HTTP/1.0 200 OK\r\n");

  process.Fork(
    [&conn, &parser, &input, &output]
    {
      input.CloseReadEnd();
      output.CloseWriteEnd();

      auto input_buffer = conn->GetInputBuffer();
      if (parser.GetMethod() == HttpMethod::kPost) {
        input.Write(input_buffer->ToStringView()
          .substr(0, parser.GetCacheContentLength()), parser.GetCacheContentLength());
      }

      char buf[4096];MemoryZero(buf);
      size_t readn = 0; 
      while ((readn = output.Read(buf)) > 0) {
        LOG_DEBUG << StringView(buf, readn);
        conn->Send(buf, readn);
      }
    },
    [&input, &parser, &output]()
    {
      char method_env[4096];
      char query_env[4096];
      char length_env[4096];

      const auto method = parser.GetMethod();
      const auto content_length = parser.GetCacheContentLength();

      std::string path = kRootPath_;

      ::snprintf(method_env, sizeof method_env, "REQUEST_METHOD=%s", GetMethodString(method));
      ::putenv(method_env);

      LOG_DEBUG << "method_env = " << method_env;

      if (parser.GetMethod() == HttpMethod::kGet) {
        auto query_url = StringView(parser.GetUrl());
        const auto delimter = query_url.find('?');
        auto args = query_url.substr(delimter).ToString();
        path += query_url.substr(0, delimter).ToString();

        LOG_DEBUG << "query_url = " << query_url;
        LOG_DEBUG << "path = " << path;
        LOG_DEBUG << "args = " << args;

        ::snprintf(query_env, sizeof query_env, "QUERY_STRING=%s", args.data());
        ::putenv(query_env);

        LOG_DEBUG << "query_env = " << query_env;
      }
      else if (method == HttpMethod::kPost) {
        path += parser.GetUrl();
        
        LOG_DEBUG << path;
        assert(path.find('?') == StringView::npos);
        ::snprintf(length_env, sizeof length_env, "CONTENT_LENGTH=%lu", content_length);
        ::putenv(length_env);
      }


      input.RedirectReadEnd(STDIN_FILENO);
      output.RedirectWriteEnd(STDOUT_FILENO);

      input.CloseWriteEnd();
      output.CloseReadEnd();

      if (::execl(path.c_str(), "", NULL) == -1) {
        ::perror("execl() error occurred");
      }
      ::exit(0);
    });
}

void HttpServer::SendErrorResponse(
  TcpConnectionPtr const& conn,
  HttpStatusCode code,
  StringView msg)
{
  // conn->SetWriteCompleteCallback(
  //   [](TcpConnectionPtr const& conn)
  //   {
  //     conn->SetWriteCompleteCallback(WriteCompleteCallback());
  //     conn->ShutdownWrite();
  //   });
  conn->Send(GetClientError(code, msg).GetBuffer());
}