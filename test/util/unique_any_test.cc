#include "kanon/util/unique_any.h"

#include <assert.h>
#include <memory>

using namespace kanon;

int main()
{
  UniqueAny obj(std::unique_ptr<int>(new int(42)));
  auto p = AnyCast<std::unique_ptr<int>>(obj);
  assert(p);
  assert(**p == 42);
}
