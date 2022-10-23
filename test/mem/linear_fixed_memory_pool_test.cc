#include "kanon/mem/linear_fixed_memory_pool.h"

#include "kanon/util/measure_time.h"

#include <gtest/gtest.h>
#include <vector>

using namespace kanon;

#define N 20

int main()
{
  LinearFixedMemoryPool<int> memory_pool(N);
  measureTime<TimeType::milliseconds>([&]() 
  {  
    std::vector<int*> arr(N);
    for (int i = 0; i < N; ++i) {
      auto p = memory_pool.Malloc();
      ASSERT_TRUE(p);
      *p = i;
      arr[i] = p;
      memory_pool.Free(arr[i]);
    }
    
    for (int i = 0; i < N; ++i) {
    }
  }, "MemoryPool");
  
  measureTime<TimeType::milliseconds>([]() 
  {  
    std::vector<int*> arr(N);
    for (int i = 0; i < N; ++i) {
      auto p = (int*)malloc(sizeof(int));
      // if (!p) memory_pool.Reserve(memory_pool.size() * 2);
      EXPECT_TRUE(p) << i;
      *p = i;
      arr[i] = p;
      free(arr[i]);
    }
    
    for (int i = 0; i < N; ++i) {
    }
  }, "Plain");
}
