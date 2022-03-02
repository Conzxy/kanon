#include "http_constant.h"
#include "kanon/util/macro.h"
#include "kanon/util/mem.h"

#include "http_response.h"

IMPORT_NAMESPACE(kanon);
IMPORT_NAMESPACE(std);

namespace http {

Buffer& HttpResponse::GetBuffer() 
{
  if (!has_length_) {
    char buf[128];
    MemoryZero(buf);
    ::snprintf(buf, sizeof buf, "%lu", body_.GetReadableSize());
    AddHeader("Content-Length", buf);
    buffer_.Append("\r\n");
    buffer_.Append(body_.ToStringView());
  }

  return buffer_;
}

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
    .AddBlackLine()
    .AddBody("<html>")
    .AddBody("<title>Kanon Error</title>")
    .AddBody("<body bgcolor=\"#ffffff\">")
    .AddBody(buf, "<h1 align=\"center\">%d %s</h1>\r\n", GetStatusCode(status_code), GetStatusCodeString(status_code))
    // .AddBody(buf, "<font size=\"7\">%d %s</font>\r\n", GetStatusCode(status_code), GetStatusCodeString(status_code))
    .AddBody(buf, "<p>%s</p>\r\n", msg.data())
    .AddBody("<div>")
    .AddBody(buf, "<center><hr><em>This is a simple http server(Kanon)</em></center>")
    .AddBody("</div>")
    .AddBody("</body>")
    .AddBody("</html>\r\n");

}
} // namespace http