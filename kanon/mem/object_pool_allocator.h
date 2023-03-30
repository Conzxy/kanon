#ifndef KANON_OBJECT_POOL_ALLOCATOR_H__
#define KANON_OBJECT_POOL_ALLOCATOR_H__

#include "fixed_chunk_memory_pool.h"

#include "kanon/zstl/type_traits.h"

namespace kanon {

template <typename T>
class ObjectPoolAllocator {
  template <typename U>
  friend class ObjectPoolAllocator;
 public:
  using value_type = T;
  using pointer = T*;
  using const_pointer = T const*;
  using reference = T&;
  using const_reference = T const&;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  
  template<typename U>
  struct rebind {
    using other = ObjectPoolAllocator<U>;
  };

  explicit ObjectPoolAllocator(FixedChunkMemoryPool &pool)
    : pool_(&pool)
  {
  }
  
  template <typename U>
  ObjectPoolAllocator(ObjectPoolAllocator<U> const &rhs)
    : pool_(rhs.pool_)
  {
  }
  
  ObjectPoolAllocator(ObjectPoolAllocator const &) = default;

  ObjectPoolAllocator &operator=(ObjectPoolAllocator const &) = default;

  T *allocate(size_type n)
  {
    if (n > 1) throw "";
    return reinterpret_cast<T*>(pool_->Malloc(sizeof(T)));
  }

  void deallocate(pointer p, size_type n) KANON_NOEXCEPT {
    (void)n;
    pool_->Free(p);
  } 

  template<typename... Args>
  void construct(pointer p, Args&&... args) {
    new(p) value_type(std::forward<Args>(args)...);
  }

  void destroy(pointer p) {
    destroy_impl(p);
  }

 private:
  template<typename U, zstl::enable_if_t<std::is_trivial<U>::value, int> =0>
  void destroy_impl(U* p) {
    // do nothing
    (void)p;
  }

  template<typename U, zstl::enable_if_t<!std::is_trivial<U>::value, char> =0>
  void destroy_impl(U* p) {
    p->~U();
  }
 
  template <typename U>
  friend bool operator==(ObjectPoolAllocator const &l, ObjectPoolAllocator<U> const &r) KANON_NOEXCEPT
  { return l.pool_ == r.pool_; }
  
  template <typename U>
  friend bool operator!=(ObjectPoolAllocator const &l, ObjectPoolAllocator<U> const &r) KANON_NOEXCEPT
  { return !(l == r); }

 private:
  FixedChunkMemoryPool *pool_;
};

template<>
class ObjectPoolAllocator<void> {
  template <typename U>
  friend class ObjectPoolAllocator;
 public:
  using value_type = void;
  using pointer = void*;
  using const_pointer = void const*;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  explicit ObjectPoolAllocator(FixedChunkMemoryPool &pool)
    : pool_(&pool)
  {
    (void)pool_;
  }
  
  template<typename U>
  struct rebind {
    using other = ObjectPoolAllocator<U>;
  };

 private:
  FixedChunkMemoryPool *pool_;
};

} // namespace kanon

#endif // KANON_OBJECT_POOL_ALLOCATOR_H__
