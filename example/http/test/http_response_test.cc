#include "../http_response.h"

#include <iostream>

IMPORT_NAMESPACE( http );

int main() {
  auto response = GetClientError(HttpStatusCode::k400BadRequest, "Bad Request");
  
  auto str = response.GetBuffer().RetrieveAllAsString();

  std::cout << str;
}
