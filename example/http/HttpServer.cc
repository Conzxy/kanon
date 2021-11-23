#include "HttpServer.h"
#include "HttpMacro.h"
#include "HttpResponse.h"

#include "kanon/log/Logger.h"
#include "kanon/net/InetAddr.h"
#include "kanon/util/macro.h"
#include "kanon/net/TcpConnection.h"
#include "kanon/util/unique_ptr.h"

#include <boost/bimap.hpp>

IMPORT_NAMESPACE( kanon );
IMPORT_NAMESPACE( http );
IMPORT_NAMESPACE( std );


static int readComplete(FILE* fp, char* buf, size_t n) {
  ssize_t readn = 0;
  size_t read_total = 0;

  while (n > 0) {
    readn = ::fread(buf, 1, n, fp);

    if (readn < 0) {
      if (errno == EINTR) {
        continue;
      }

      LOG_SYSERROR << "Fialed in calling ::fread()";

      return -1;
    } else if (readn == 0) {
      return read_total;
    }
  
    n -= readn;
    read_total += readn;
    
  }

  return 0;
}

typedef std::shared_ptr<FILE> FilePtr;
static std::map<TcpConnection*, std::weak_ptr<FILE>> g_connFile;

//static std::map<std::string, FilePtr> g_files;
typedef boost::bimap<std::string, FilePtr> PathFilePtrBM;
static PathFilePtrBM g_files;

void handleGetMethod(TcpConnectionPtr const& conn, StringView path) {
  std::string path_str{ path.data(), path.size() };
  auto iter = g_files.left.find(path_str);
  
  FilePtr fp;

  if (iter != g_files.left.end()) {
    fp = iter->second;
    assert(fp);
  } else {
    fp.reset(::fopen(path_str.c_str(), "rb"), ::fclose);
    g_files.insert(PathFilePtrBM::value_type{
        std::move(path_str),
        fp});

    if (!fp) {
      conn->send(http::clientError(HttpStatusCode::k404NotFound, "The file is not exists in server").buffer());
      conn->shutdownWrite();
      return;
    }
  }

  g_connFile.emplace(getPointer(conn), fp);

  char buf[FILE_BUFFER_SIZE];
  
  // Although short read happend, the writeCompleteCallback
  // will read more
  int readn = ::fread(buf, 1, sizeof buf, getPointer(fp));
  
  if (readn > 0) {
    conn->send(buf, readn);
  } else if (readn < 0) {
    LOG_ERROR << "Failed in ::fread()";
    g_connFile.erase(getPointer(conn));
    g_files.right.erase(fp); 
    // FIXME retry
    conn->shutdownWrite();
  } 
}

HttpServer::HttpServer(EventLoop* loop, InetAddr const& listen_addr)
  : server_(loop, listen_addr, "HttpServer")
  , parser_()
{
  server_.setConnectionCallback([](TcpConnectionPtr const& conn) {
    defaultConnectionCallaback(conn);
    if (conn->isConnected()) {
      conn->setContext(new HttpRequestParser{});
    } else {
      delete *unsafeAnyCast<HttpRequestParser*>(&conn->context());
    }
  });

  server_.setMessageCallback([](TcpConnectionPtr const& conn,
        Buffer& buffer, TimeStamp receive_time) {
      KANON_UNUSED(receive_time);
      auto p_parser = unsafeAnyCast<HttpRequestParser*>(&conn->context());
      assert(p_parser);

      auto parser = *p_parser;

      if (ParseResult::kComplete == parser->parse(conn, buffer)) {
        auto path = std::move(parser->path());

        switch (parser->method()) {
        case HttpMethod::kGet:
          handleGetMethod(conn, path);
          break;
        case HttpMethod::kPut:
          break;
        case HttpMethod::kPost:
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
          LOG_ERROR << "Method should not be NotSupport here";
          break;
        default:
          break;
        }

      }
  });
  
  server_.setWriteCompleteCallback([](TcpConnectionPtr const& conn) {
    // FIXME provide method strings array
    LOG_INFO << "Write Complete(GET)";
    
    auto conn_file = g_connFile.find(getPointer(conn));

    if (conn_file != g_connFile.end()) {
      char buf[FILE_BUFFER_SIZE];
      FilePtr fp = conn_file->second.lock();
      
      auto readn = ::fread(buf, 1, sizeof buf, getPointer(fp));
      
      if (readn > 0) { 
        conn->send(buf, readn);
      } else { // readn <= 0
        if (readn < 0) {
          LOG_ERROR << "Failed in ::fread()";
        }

        LOG_INFO << "fp count: " << fp.use_count();

        if (fp.use_count() <= 3) {
          g_files.right.erase(fp);
        }

        g_connFile.erase(getPointer(conn));
        conn->shutdownWrite();
      }
    }
  });
}