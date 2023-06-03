#include "kanon/mem/fixed_chunk_memory_pool.h"
#include "kanon/util/measure_time.h"

#include <vector>

using namespace kanon;

class Pool_c { // Basic type define
  typedef unsigned int uint;
  typedef unsigned char uchar;
  uint m_numOfBlocks;     // Num of blocks
  uint m_sizeOfEachBlock; // Size of each block
  uint m_numFreeBlocks;   // Num of remaining blocks
  uint m_numInitialized;  // Num of initialized blocks
  uchar *m_memStart;      // Beginning of memory pool
  uchar *m_next;          // Num of next free block
 public:
  Pool_c()
  {
    m_numOfBlocks = 0;
    m_sizeOfEachBlock = 0;
    m_numFreeBlocks = 0;
    m_numInitialized = 0;
    m_memStart = NULL;
    m_next = 0;
  }
  ~Pool_c() { DestroyPool(); }
  void CreatePool(size_t sizeOfEachBlock, uint numOfBlocks)
  {
    m_numOfBlocks = numOfBlocks;
    m_sizeOfEachBlock = sizeOfEachBlock;
    m_memStart = new uchar[m_sizeOfEachBlock * m_numOfBlocks];
    m_numFreeBlocks = numOfBlocks;
    m_next = m_memStart;
  }
  void DestroyPool()
  {
    delete[] m_memStart;
    m_memStart = NULL;
  }
  uchar *AddrFromIndex(uint i) const
  {
    return m_memStart + (i * m_sizeOfEachBlock);
  }
  uint IndexFromAddr(const uchar *p) const
  {
    return (((uint)(p - m_memStart)) / m_sizeOfEachBlock);
  }
  void *Allocate()
  {
    if (m_numInitialized < m_numOfBlocks) {
      uint *p = (uint *)AddrFromIndex(m_numInitialized);
      *p = m_numInitialized + 1;
      m_numInitialized++;
    }
    void *ret = NULL;
    if (m_numFreeBlocks > 0) {
      ret = (void *)m_next;
      --m_numFreeBlocks;
      if (m_numFreeBlocks != 0) {
        m_next = AddrFromIndex(*((uint *)m_next));
      } else {
        m_next = NULL;
      }
    }
    return ret;
  }
  void DeAllocate(void *p)
  {
    if (m_next != NULL) {
      (*(uint *)p) = IndexFromAddr(m_next);
      m_next = (uchar *)p;
    } else {
      *((uint *)p) = m_numOfBlocks;
      m_next = (uchar *)p;
    }
    ++m_numFreeBlocks;
  }
};
struct A {
  char data[400];
};

#define N 20000000
#define M (int(N / sizeof(A)))

int main()
{
  measureTime<TimeType::milliseconds>(
      [&]() {
        std::vector<int *> arr(N);
        FixedChunkMemoryPool memory_pool(1000);
        for (int i = 0; i < N; ++i) {
          auto p = (int *)memory_pool.Malloc(sizeof(int));
          *p = i;
          arr[i] = p;
        }

        for (int i = 0; i < N; ++i) {
          memory_pool.Free(arr[i]);
        }
      },
      "MemoryPool");

  measureTime<TimeType::milliseconds>(
      []() {
        std::vector<int *> arr(N);
        for (int i = 0; i < N; ++i) {
          auto p = (int *)malloc(sizeof(int));
          *p = i;
          arr[i] = p;
        }

        for (int i = 0; i < N; ++i) {
          free(arr[i]);
        }
      },
      "Plain");

  measureTime<TimeType::milliseconds>(
      []() {
        std::vector<void *> arr(M);
        FixedChunkMemoryPool memory_pool(10);
        for (int i = 0; i < M; ++i) {
          auto p = memory_pool.Malloc(sizeof(A));
          arr[i] = p;
        }

        for (int i = 0; i < M; ++i) {
          memory_pool.Free(arr[i]);
        }
      },
      "MemoryPool Big");

  measureTime<TimeType::milliseconds>(
      []() {
        std::vector<void *> arr(M);
        Pool_c memory_pool;
        memory_pool.CreatePool(sizeof(A), M);
        for (int i = 0; i < M; ++i) {
          auto p = memory_pool.Allocate();
          arr[i] = p;
        }

        auto p = memory_pool.Allocate();
        if (!p) {
          puts("Error");
        }
        for (int i = 0; i < M; ++i) {
          memory_pool.DeAllocate(arr[i]);
        }
      },
      "MemoryPool(2) Big");

  measureTime<TimeType::milliseconds>(
      []() {
        std::vector<void *> arr(M);
        for (int i = 0; i < M; ++i) {
          auto p = malloc(sizeof(A));
          arr[i] = p;
        }

        for (int i = 0; i < M; ++i) {
          free(arr[i]);
        }
      },
      "Plain Big");
}

