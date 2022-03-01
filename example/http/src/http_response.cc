#include "kanon/util/macro.h"
#include "kanon/util/mem.h"

#include "http_response.h"

IMPORT_NAMESPACE(kanon);
IMPORT_NAMESPACE(std);

namespace http {

HttpResponse GetClientError(
  HttpStatusCode status_code,
  StringView msg)
{
  HttpResponse response;
  char buf[4096];MemoryZero(buf);

  // Content-Length will compute automatically and append
  return response
    .AddHeaderLine(status_code)
    .AddHeader("Content-Type", "text/html")
    .AddHeader("Connection", "close")
    .AddHeaderBlackLine()
    .AddBody("<html>")
    .AddBody("<title>Kanon Error</title>")
    .AddBody("<body bgcolor=\"#ffffff\">")
    .AddBody(buf, "<font size=\"7\">%d %s</font>\r\n", GetStatusCode(status_code), GetStatusCodeString(status_code))
    .AddBody(buf, "<p>%s</p>\r\n", msg.data())
    .AddBody(buf, "<hr><em>This is a simple http server(Kanon)</em>")
    .AddBody("</body>")
    .AddBody("</html>\r\n");

}
} // namespace http