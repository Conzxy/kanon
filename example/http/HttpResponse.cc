#include "HttpResponse.h"

#include "kanon/util/macro.h"

IMPORT_NAMESPACE(http);
IMPORT_NAMESPACE(kanon);
IMPORT_NAMESPACE(std);


HttpResponse
http::clientError(HttpStatusCode status_code,
    StringView msg) {
  HttpResponse response;
  char buf[4096];

  response.addVersion(HttpVersion::kHttp10);
  response.addStatusCodeAndString(status_code);
  response.addHeader("Content-Type", "text/html");
  response.addBlankLine();
  
  response.addBody("<html><title>Error</title>");
  response.addBody("<body bgcolor=""ffffff"">\r\n");

  ::snprintf(buf, sizeof buf, "%d: %s\r\n", 
      getStatusCode(status_code), getStatusCodeString(status_code));
  response.addBody(buf);

  ::snprintf(buf, sizeof buf, "<p>%.*s\r\n", 
      msg.size(), msg.data());
  response.addBody(buf);

  ::snprintf(buf, sizeof buf, 
      "<hr><em>The Kanon Web Server</em>\r\n");
  response.addBody(buf);
  
  return response;
}
