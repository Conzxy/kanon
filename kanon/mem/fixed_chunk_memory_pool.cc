#include "fixed_chunk_memory_pool.h"

#include <stdlib.h> // malloc(), free()
#include <stddef.h> // offsetof(), size_t
#include <assert.h> // assert()

using namespace kanon;

// #define ROUNDUP4(x) (((((x) - 1) >> 2) + 1) << 2)
// #define ROUNDUP8(x) (((((x) - 1) >> 3) + 1) << 3)

#define CHUNK_NEXT_OFFSET (offsetof(Chunk, next))
#define BLOCK_SPACE(sz) (sizeof(Block) + (CHUNK_NEXT_OFFSET + sz) * chunk_per_block_)
#define ALIGN_SIZE(sz) ((sz < chunk_next_space) ? chunk_next_space : sz)
#define CHECK_ALIGNED(sz) (sz >= 8)

FixedChunkMemoryPool::FixedChunkMemoryPool(size_t chunk_per_block)
  : chunk_per_block_(chunk_per_block)
  , free_chunk_header_(nullptr)
  , block_chain_(nullptr)
  , block_count_(0)
{
}

FixedChunkMemoryPool::~FixedChunkMemoryPool() noexcept
{
  Block *next = nullptr;
  while (block_chain_) {
    next = block_chain_->next;
    free(block_chain_);
    block_chain_ = next;
  }
}

void *FixedChunkMemoryPool::Malloc(size_t sz)
{
  if (sz == 0) return nullptr;

  /* Align to the size of pointer at least
   * since the space of `next` must greater than the object size */
  sz = ALIGN_SIZE(sz);

  Chunk *header = free_chunk_header_;
  /* There are no block */
  if (!header) {
    header = AllocBlock(sz);
  }
  
  /* Recheck header in case of lack of memory */
  if (!header) {
    assert(!free_chunk_header_);
    return nullptr;
  }

  free_chunk_header_ = free_chunk_header_->next;
  header->status = ALLOCATED;
  return reinterpret_cast<char*>(header)+CHUNK_NEXT_OFFSET;
}

void FixedChunkMemoryPool::Free(void *ptr)
{
  if (!ptr) return;
  /* Back the chunk to the pool */
  auto new_free_chunk = reinterpret_cast<Chunk*>(reinterpret_cast<char*>(ptr)-CHUNK_NEXT_OFFSET);
  assert(new_free_chunk->status == ALLOCATED);
  new_free_chunk->status = FREE;
  new_free_chunk->next = free_chunk_header_;
  free_chunk_header_ = new_free_chunk;
}

void FixedChunkMemoryPool::Shrink(size_t sz)
{
  sz = ALIGN_SIZE(sz);
  /* Check the block whether it can be freed */
  Block *tmp_next = nullptr;

  /* First, Check the block header whether can free */
  while (IsBlockFree(block_chain_, sz)) {
    tmp_next = block_chain_->next;
    free(block_chain_);
    block_chain_ = tmp_next;
    block_count_--;
  }
  
  /* The first block is allocated, we can redirect the next don't care it */
  for (auto block_iter = block_chain_; block_iter->next != nullptr; block_iter = block_iter->next) {
    tmp_next = block_iter->next;
    if (IsBlockFree(tmp_next, sz)) {
      block_iter->next = tmp_next->next;
      free(tmp_next);
      block_count_--;
    }
  }
}

size_t FixedChunkMemoryPool::GetUsage(size_t sz) const noexcept
{
  return BLOCK_SPACE(ALIGN_SIZE(sz)) * block_count_;
}

size_t FixedChunkMemoryPool::GetFreeChunkNum(size_t sz) const noexcept
{
  sz = ALIGN_SIZE(sz);
  auto block_iter = block_chain_;
  size_t ret = 0;
  for (; block_iter != nullptr; block_iter = block_iter->next) {
    ret += GetFreeChunkNumInBlock(block_iter, sz);
  }
  return ret;
}

auto FixedChunkMemoryPool::AllocBlock(size_t sz) -> Chunk*
{
  assert(CHECK_ALIGNED(sz));
  auto header = reinterpret_cast<Block*>(malloc(BLOCK_SPACE(sz)));
  if (!header) return nullptr;
  
  /* Link the block */
  header->next = block_chain_;
  block_chain_ = header;

  free_chunk_header_ = reinterpret_cast<Chunk*>(reinterpret_cast<char*>(header)+sizeof(Block));
  free_chunk_header_->status = FREE;
  
  /* Link the chunks */
  auto chunk_header = free_chunk_header_;
  for (size_t i = 1; i < chunk_per_block_; ++i) {
    chunk_header->next = reinterpret_cast<Chunk*>(reinterpret_cast<char*>(chunk_header) + CHUNK_NEXT_OFFSET + sz);
    chunk_header = chunk_header->next;
    chunk_header->status = FREE;
  }
  
  chunk_header->next = nullptr;
  block_count_++;
  return free_chunk_header_;
}

bool FixedChunkMemoryPool::IsBlockFree(Block *block, size_t sz)
{
#if 0
  return GetFreeChunkNumInBlock(block, sz) == chunk_per_block_;
#else
  assert(CHECK_ALIGNED(sz));
  auto chunk_iter = reinterpret_cast<Chunk*>(reinterpret_cast<char*>(block) + sizeof(Block));
  for (size_t i = 0; i < chunk_per_block_; ++i) {
    if (chunk_iter->status == ALLOCATED)
      return false;
    chunk_iter = reinterpret_cast<Chunk*>(reinterpret_cast<char*>(chunk_iter) + CHUNK_NEXT_OFFSET + sz);
  }
  return true;
#endif
}

size_t FixedChunkMemoryPool::GetFreeChunkNumInBlock(Block *block, size_t sz) const noexcept
{
  assert(CHECK_ALIGNED(sz));
  auto chunk_iter = reinterpret_cast<Chunk*>(reinterpret_cast<char*>(block) + sizeof(Block));
  size_t ret = 0;
  for (size_t i = 0; i < chunk_per_block_; ++i) {
    if (chunk_iter->status == FREE)
      ret++;
    chunk_iter = reinterpret_cast<Chunk*>(reinterpret_cast<char*>(chunk_iter) + CHUNK_NEXT_OFFSET + sz);
  }

  return ret;
}
