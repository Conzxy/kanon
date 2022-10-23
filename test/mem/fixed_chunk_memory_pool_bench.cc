#include "kanon/mem/fixed_chunk_memory_pool.h"
#include "kanon/util/measure_time.h"

#include <vector>

using namespace kanon;

struct A {
  char data[400];
};

#define N 20000000
#define M (int(N / sizeof(A)))

int main()
{
  measureTime<TimeType::milliseconds>([&]() 
  {  
    std::vector<int*> arr(N);
    FixedChunkMemoryPool memory_pool(1000);
    for (int i = 0; i < N; ++i) {
      auto p = (int*)memory_pool.Malloc(sizeof(int));
      *p = i;
      arr[i] = p;
    }
    
    for (int i = 0; i < N; ++i) {
      memory_pool.Free(arr[i]);
    }
  }, "MemoryPool");
  
  measureTime<TimeType::milliseconds>([]() 
  {  
    std::vector<int*> arr(N);
    for (int i = 0; i < N; ++i) {
      auto p = (int*)malloc(sizeof(int));
      *p = i;
      arr[i] = p;
    }
    
    for (int i = 0; i < N; ++i) {
      free(arr[i]);
    }
  }, "Plain");

  measureTime<TimeType::milliseconds>([]()
  {
    std::vector<void*> arr(M);
    FixedChunkMemoryPool memory_pool(100);
    for (int i = 0; i < M; ++i) {
      auto p = memory_pool.Malloc(sizeof(A));
      arr[i] = p;
    }
    
    for (int i = 0; i < M; ++i) {
      memory_pool.Free(arr[i]);
    }
  }, "MemoryPool Big");

  measureTime<TimeType::milliseconds>([]()
  {
    std::vector<void*> arr(M);
    for (int i = 0; i < M; ++i) {
      auto p = malloc(sizeof(A));
      arr[i] = p;
    }
    
    for (int i = 0; i < M; ++i) {
      free(arr[i]);
    }
  }, "Plain Big");
}

