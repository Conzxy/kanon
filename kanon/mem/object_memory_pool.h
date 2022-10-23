#ifndef KANON_MEM_OBJECT_MEMORY_POOL_H__
#define KANON_MEM_OBJECT_MEMORY_POOL_H__

#include "fixed_chunk_memory_pool.h"

namespace kanon {

template <typename T>
class ObjectMemoryPool {
 public:
  explicit ObjectMemoryPool(size_t chunk_per_block=1)
    : base_(chunk_per_block)
  {}
  
  ~ObjectMemoryPool() noexcept = default;
  ObjectMemoryPool(ObjectMemoryPool const &) = delete;
  ObjectMemoryPool(ObjectMemoryPool &&) = delete;

  ObjectMemoryPool &operator=(ObjectMemoryPool const &) = delete;
  ObjectMemoryPool &operator=(ObjectMemoryPool &&) = delete;

  void *Malloc()
  {
    return base_.Malloc(sizeof(T));
  }

  void Free(void *ptr)
  {
    return base_.Free(ptr);
  }

  void Shrink()
  {
    return base_.Shrink(sizeof(T));
  }

  size_t GetUsage() const noexcept
  {
    return base_.GetUsage(sizeof(T));
  }

 private:
  FixedChunkMemoryPool base_;
};

} // namespace kanon

#endif // KANON_MEM_OBJECT_MEMORY_POOL_H__
