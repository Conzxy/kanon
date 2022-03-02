#include "http_server.h"

#include "kanon/log/async_log_trigger.h"

using namespace http;

int main() {
  kanon::AsyncLogTrigger::instance(
    "http_server",
    1024 * 1024 * 2,
    "/root/.log/http/");

  EventLoop loop;
  HttpServer server(&loop);
  
  server.StartRun();

  loop.StartLoop();
}