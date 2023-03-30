#ifndef _KANON_ALGO_LIBC_ALLOCATOR_WITH_REALLOC_H_
#define _KANON_ALGO_LIBC_ALLOCATOR_WITH_REALLOC_H_

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <stdlib.h>
#include <type_traits>
#include <utility>

#include "kanon/zstl/type_traits.h"

namespace kanon {
namespace algo {

/**
  * 在C++中一般内存管理是通过new和delete运算符进行。
  * 尽管在一些实现中，new和delete底层是转发的::malloc()和::free(),
  * libc的realloc()并不一定与new和delete兼容，从而导致UB，
  * 同时，标准的std::allocator<>并没有提供reallocate()接口支持重分配。
  * 因此STL一般的策略是destroy()->deallocate()->allocate()->construct()，
  * 但采用libc的realloc()可以在一些场景下进行原地扩展或收缩，最坏情况才会按一般策略来，
  * 之所以诟病realloc()，是因为对于non-trivial类型来说，直接进行bitwise-move是错误，危险的
  * 对于trivial类型和一些可以当作non-trivial类型对待的trivial类型而言，或许是不错的小优化。
  * \see 
  */
template<typename T>
class LibcAllocatorWithRealloc {
 public:
  using value_type = T;
  using pointer = T*;
  using const_pointer = T const*;
  using reference = T&;
  using const_reference = T const&;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  // 尽管C++11支持模板别名
  // 兼容11之前的标准
  template<typename U>
  struct rebind {
    using other = LibcAllocatorWithRealloc<U>;
  };

  LibcAllocatorWithRealloc() = default;
  
  pointer address(reference x) const KANON_NOEXCEPT { return &x; }

  const_pointer address(const_reference x) KANON_NOEXCEPT { return &x; }
  
  T* allocate(size_type n) KANON_NOEXCEPT {
    return reinterpret_cast<T*>(::malloc(n*sizeof(T))); 
  }
  
  T* reallocate(pointer p, size_type n) KANON_NOEXCEPT {
    return reinterpret_cast<T*>(::realloc(p, n*sizeof(T))); 
  } 

  void deallocate(pointer p, size_type n) KANON_NOEXCEPT {
    (void)n;
    // If p is NULL, free() do nothing
    ::free(p);
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
  // stateless
};

// void特化：
// 无成员函数及reference，const_reference
template<>
class LibcAllocatorWithRealloc<void> {
 public:
  using value_type = void;
  using pointer = void*;
  using const_pointer = void const*;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  template<typename U>
  struct rebind {
    using other = LibcAllocatorWithRealloc<U>;
  };
};

} // namespace algo
} // namespace kanon

#endif // _KANON_ALGO_LIBC_ALLOCATOR_WITH_REALLOC_H_
