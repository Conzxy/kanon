#include "../HttpResponse.h"

#include <iostream>

IMPORT_NAMESPACE( http );

int main() {
  auto response = clientError(HttpStatusCode::k400BadRequest, "Bad Request");
  
  auto str = response.buffer().retrieveAllAsString();

  std::cout << str;
}
