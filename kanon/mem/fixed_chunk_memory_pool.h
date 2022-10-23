#ifndef KANON_MEM_FIXED_CHUNK_MEMORY_POOL_H_
#define KANON_MEM_FIXED_CHUNK_MEMORY_POOL_H_

#include <stddef.h> // size_t

namespace kanon {

/**
 * \brief A efficient memory pool based on fixed size chunk
 *
 * The memory pool also can support shrink.
 *
 * The memory contains `Block` and `Chunk`.
 *
 * The Chunk is a fixed size space, if it is free, contains a pointer to the next free chunk,
 * otherwise it contains the user-defined object(or nested type).
 *
 * The Block is a collection of preallocated chunks, to make allocation faster.
 *
 * The Chunk is a allocation unit for user, and Block is a allocation unit for malloc().
 * 
 * The outline as following:
 * |+++++++++++++++++++++++++++++++++++|     |+++++++++++++++++++++++++++++++++++|
 * |             Block                 |     |             Block                 |
 * | |+++++++|    |+++++++|            |     | |+++++++|    |+++++++|            |
 * | | chunk | -> | chunk | -> nullptr | --> | | chunk | -> | chunk | -> nullptr | --> nullptr
 * | |  a/f  |    |  a/f  |            |     | |  a/f  |    |  a/f  |            |
 * | |+++++++|    |+++++++|            |     | |+++++++|    |+++++++|            |
 * |                                   |     |                                   |
 * |+++++++++++++++++++++++++++++++++++|     |+++++++++++++++++++++++++++++++++++|
 *
 * \warning
 *  This is not a generic memory pool(ie. don't support variable size chunk allocation).
 */
class FixedChunkMemoryPool {
  enum ChunkStatus : size_t {
    ALLOCATED = 0,
    FREE,
  };

  struct Chunk {
    ChunkStatus status;
    Chunk *next;
  };

  struct Block {
    Block *next;
  };

  static constexpr size_t chunk_next_space = sizeof(Chunk) - offsetof(Chunk, next);
  
 public:
  /**
   * \brief Construct a pool in \p chunk_per_block
   *
   * \param chunk_per_block The number of the chunk(preallocated space) in a block
   */
  explicit FixedChunkMemoryPool(size_t chunk_per_block=1);

  ~FixedChunkMemoryPool() noexcept;
  
  FixedChunkMemoryPool(FixedChunkMemoryPool const &) = delete;
  FixedChunkMemoryPool(FixedChunkMemoryPool &&) = delete;

  FixedChunkMemoryPool &operator=(FixedChunkMemoryPool const &) = delete;
  FixedChunkMemoryPool &operator=(FixedChunkMemoryPool &&) = delete;

  /**
   * \brief Malloc a chunk whose size equal to sz
   *
   * \param sz The size of a Chunk or Object(If sz == 0, do nothing)
   * \return 
   *  nullptr -- Failed to malloc a chunk(lack of memory)
   *
   *  Otherwise -- successful.
   */
  void *Malloc(size_t sz);

  /**
   * \brief Free a chunk
   * 
   * In fact, just back the chunk to pool.
   * Don't free the chunk throught glibc's free().
   */
  void Free(void *ptr);
  
  /**
   * \brief Shrink the pool
   *
   * Check the free block and free it
   *
   * \note O(n), n is the count of chunk
   */
  void Shrink(size_t sz);
  
  /**
   * \brief Get the memory usage of pool, i.e. allocated memory size
   *
   * \warning 
   *  If object size less than 8, it will be aligned to the multiplier of 8
   */
  size_t GetUsage(size_t sz) const noexcept;
  
  /** For debugging and test */
  size_t GetBlockNum() const noexcept { return block_count_; }
  size_t GetChunkPerBlock() const noexcept { return chunk_per_block_; }

  /** \warning O(n), n is the number of chunk */
  size_t GetFreeChunkNum(size_t sz) const noexcept;
  
  void SetChunkPerBlock(size_t n) noexcept { chunk_per_block_ = n; }
 private:
  /**
   * \return 
   *  nullptr -- Failed to allocate block
   */
  Chunk *AllocBlock(size_t sz);
  
  /**
   * Check the block whether to free
   */
  bool IsBlockFree(Block *block, size_t sz);
  
  size_t GetFreeChunkNumInBlock(Block *block, size_t sz) const noexcept;


  size_t chunk_per_block_;
  Chunk *free_chunk_header_;
  Block *block_chain_;
  size_t block_count_;
};
 
} // namespace kanon

#endif // KANON_MEM_FIXED_CHUNK_MEMORY_POOL_H_ 
