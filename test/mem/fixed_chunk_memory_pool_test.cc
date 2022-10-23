#include "kanon/mem/fixed_chunk_memory_pool.h"

#include <stdlib.h>
#include <gtest/gtest.h>

#include "leak_detector.h"

using namespace kanon;

TEST (fixed_chunk_memory_pool, allocation1)
{
  using Object = int;
  auto obj_size = sizeof(Object);
  int n = 10;
  FixedChunkMemoryPool pool(n);
  for (int i = 0; i < n; ++i) {
    auto p = (Object*)pool.Malloc(obj_size);
    *p = i;
    ASSERT_EQ(*p, i);
  }
  
  EXPECT_EQ(pool.GetBlockNum(), 1);
}

TEST (fixed_chunk_memory_pool, allocation2)
{
  using Object = int;
  auto obj_size = sizeof(Object);
  int n = 10;
  int block_num = 10;
  FixedChunkMemoryPool pool(n);
  for (int i = 0; i < n * block_num; ++i) {
    auto p = (Object*)pool.Malloc(obj_size);
    *p = i;
    ASSERT_EQ(*p, i);
  }
  
  EXPECT_EQ(pool.GetBlockNum(), block_num);
}

TEST (fixed_chunk_memory_pool, free1)
{
  using Object = int;
  auto obj_size = sizeof(Object);
  int n = 10;
  int block_num = 1;
  FixedChunkMemoryPool pool(n);
  std::vector<void*> allocated_ptr;
  allocated_ptr.reserve(n*block_num);
  for (int i = 0; i < n * block_num; ++i) {
    auto p = (Object*)pool.Malloc(obj_size);
    *p = i;
    ASSERT_EQ(*p, i);
    allocated_ptr.push_back(p);
  }
  
  EXPECT_EQ(pool.GetBlockNum(), block_num);

  for (auto ptr : allocated_ptr) {
    pool.Free(ptr);
  }

  EXPECT_EQ(pool.GetFreeChunkNum(obj_size), n * block_num);
}

TEST (fixed_chunk_memory_pool, free2)
{
  using Object = int;
  auto obj_size = sizeof(Object);
  int n = 10;
  int block_num = 10;
  FixedChunkMemoryPool pool(n);
  std::vector<void*> allocated_ptr;
  allocated_ptr.reserve(n*block_num);
  for (int i = 0; i < n * block_num; ++i) {
    auto p = (Object*)pool.Malloc(obj_size);
    *p = i;
    ASSERT_EQ(*p, i);
    allocated_ptr.push_back(p);
  }
  
  EXPECT_EQ(pool.GetBlockNum(), block_num);

  for (auto ptr : allocated_ptr) {
    pool.Free(ptr);
  }

  EXPECT_EQ(pool.GetFreeChunkNum(obj_size), n * block_num);

  CHECK_LEAKS();
}
