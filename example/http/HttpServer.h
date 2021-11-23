#ifndef KANON_HTTPSERVER_H
#define KANON_HTTPSERVER_H

#include "kanon/util/noncopyable.h"
#include "kanon/net/TcpServer.h"

#include "HttpRequestParser.h"

namespace http {
  class HttpServer : kanon::noncopyable {
  public:
    explicit HttpServer(kanon::EventLoop* loop,
        kanon::InetAddr const& listen_addr);
      
    void start() {
      server_.start();
    } 

  private:
    kanon::TcpServer server_;
    HttpRequestParser parser_; 
  };
}

#endif // KANON_HTTPSERVER_H
