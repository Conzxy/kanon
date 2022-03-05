#include "../ptr.h"

#include <assert.h>

int main()
{
  auto p1 = kanon::make_unique<int>(32);
  auto p2 = kanon::make_unique<int[]>(32);
  
  p2[3] = 2; 

  assert(p2[3] == 2);
  auto p3 = kanon::make_unique<int[32]>(22);
}
