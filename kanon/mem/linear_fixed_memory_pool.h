#ifndef KANON_MEM_LINEAR_FIXED_MEMORY_POOL_H__
#define KANON_MEM_LINEAR_FIXED_MEMORY_POOL_H__

#include <stdlib.h>
#include <assert.h>

#include <stdexcept>
#include <unordered_set>
// #include <vector>

namespace kanon {

// #define CHECK_BIT(integer, bit) ((integer) & (1 << (bit)))
// #define SET_BIT(integer, bit) ((integer) |= (1 << (bit)))
// #define RESET_BIT(integer, bit) ((integer) &= (~(1 << bit)))

template <typename T>
class LinearFixedMemoryPool {
  static constexpr size_t PAYLOAD_SIZE = sizeof(T);
  // static constexpr size_t BLOCK_SIZE = PAYLOAD_SIZE + 1;
  static constexpr size_t BLOCK_SIZE = PAYLOAD_SIZE;
 public:
  explicit LinearFixedMemoryPool(size_t size)
    : pool_(static_cast<T*>(malloc(size * BLOCK_SIZE)))
    , size_(size)
  {
    if (pool_ == nullptr) {
      throw std::bad_alloc{};
    }

    for (size_t i = 0; i < size; ++i)
      free_hset_.insert(i);
  }
  
  ~LinearFixedMemoryPool() KANON_NOEXCEPT
  {
    free(pool_);
    size_ = 0;
  }

  T *Malloc()
  {
    // for (auto start = pool_; size_t(start - pool_) < size_ * BLOCK_SIZE; start += BLOCK_SIZE) {
    //   if (!CHECK_BIT(*start, 0)) {
    //     SET_BIT(*start, 0);
    //     assert(CHECK_BIT(*start, 0));
    //     return reinterpret_cast<T*>(start);
    //   }
    // }
    if (free_hset_.empty()) return (T*)malloc(BLOCK_SIZE);
    auto ret = *free_hset_.begin();
    free_hset_.erase(free_hset_.begin());
    return pool_+ret;
  }
  
  void Reserve(size_t sz)
  {
    if (size_ >= sz) return;

    pool_ = static_cast<T*>(reallocarray(pool_, sz, BLOCK_SIZE));

    for (size_t i = size_; i < sz; ++i) {
      free_hset_.insert(i);
    }
    size_ = sz;
  }

  void Free(T *ptr)
  {
    // assert(CHECK_BIT(*((char*)ptr-1), 0));
    // RESET_BIT(*((char*)ptr-1), 0);
    if (ptr >= pool_ && ptr < pool_ + size_) {
      free_hset_.insert(ptr-pool_);
    } else {
      free(ptr);
    }
  }
  
  size_t size() const KANON_NOEXCEPT { return size_; }
  size_t freesize() const KANON_NOEXCEPT { return free_hset_.size(); }
 private:
  T *pool_;
  std::unordered_set<size_t> free_hset_;
  size_t size_;
};

} // namespace kanon

#endif // KANON_MEM_LINEAR_FIXED_MEMORY_POOL_H__
