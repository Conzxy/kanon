#include "kanon/mem/object_pool_allocator.h"

#include "leak_detector.h"

#include <assert.h>
#include <memory>

using namespace kanon;
using namespace std;

struct A {
  explicit A(int _x) : x(_x) {}
  int x;
};

int main()
{
  kanon::FixedChunkMemoryPool pool(1);
  ObjectPoolAllocator<A> allocator(pool);

  auto p = std::allocate_shared<A>(allocator, 1);
  auto p2 = std::allocate_shared<A>(allocator, 2);

  assert(pool.GetBlockNum() == 2);
  CHECK_LEAKS();
}
