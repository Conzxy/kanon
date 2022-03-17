#ifndef KANON_ALGO_CIRCULAR_BUFFER_H
#define KANON_ALGO_CIRCULAR_BUFFER_H

#ifdef CIRCULAR_BUFFER_DEBUG
#include <map>
#endif
#include <assert.h>
#include <memory>
#include <utility>

#include "kanon/util/macro.h"

namespace kanon { 
namespace algo_util {

namespace detail {
template<typename T, typename Alloc=std::allocator<T>>
class CircularBufferBase : public Alloc {
public:
  using value_type      = T;
  using size_type       = size_t;
  using pointer         = T *;
  using const_pointer   = T const *;
  using reference       = T &;
  using const_reference = T const &;
  using iterator        = pointer;
  using const_iterator  = const_pointer;

  explicit CircularBufferBase(const size_type max_size)
  KANON_FUNCTION_TRY
    begin_{this->allocate(max_size + 1)},
    end_{begin_ + max_size + 1},
    read_{begin_},
    write_{read_}
  {
    
  }
  catch (std::bad_alloc const& ba) {
    ::fprintf(stderr,
             "required max size %luB of CircularBuffer has overed the system memory",
              max_size);
    KANON_RETHROW;
  }

  ~CircularBufferBase() noexcept
  {
    this->deallocate(begin_, end_ - begin_);
  }

  pointer begin_; // begin of allocated space
  pointer end_; // end of allocated space
  pointer read_; // read pointer
  pointer write_; // write pointer
};

} // namespace detail

/**
 * @class CircularBuffer
 * @tparam T type of element(or entry)
 * @brief
 * The difference between CircularBuffer and RingBuffer is that 
 * CircularBuffer don't use mmap() to allocate 2* size buffer, it  
 * create a illusion that memory is circular. 
 * @note 
 * CircularBuffer have a problem:
 * The predicata of buffer empty or full is same, we use the simplest 
 * approach to solve that allocation space of (size+1).
 * then if read pointer and write pointer meet write + 1 = read, 
 * we think it is full.
 * @note
 * Only want use it to implemete BoundedQueue,
 * then, I don't provide other API like std::deque.
 */
template<typename T, typename Alloc=std::allocator<T>>
class CircularBuffer {
public:
  using value_type      = T;
  using size_type       = size_t;
  using pointer         = T*;
  using const_pointer   = T const*;
  using reference       = T&;
  using const_reference = T const&;
  using iterator        = pointer;
  using const_iterator  = const_pointer;

  explicit CircularBuffer(const size_type max_size)
    : base_{max_size}
  { }

  ~CircularBuffer() = default;

  template<typename... Args>
  void emplace(Args&&... args)
  {
    if (is_full()) {
      base_.destroy(base_.read_);
      base_.construct(base_.write_, std::forward<Args>(args)...);
      AdvanceRead(1);
      advance_write(1);
      
      assert(is_full());
    }
    else {
      base_.construct(base_.write_, std::forward<Args>(args)...);
      advance_write(1);
    }
  }

  // template<typename... Args>
  // void emplace_front(Args&&... args)
  // {
  //   back_read();
  //   base_.construct(base_.read_, std::forward<Args>(args)...);
  // }

  // NOTE Don't use value_type const& and value_type&&.
  // If move is cheap, the value passed also a good choice
  void push(value_type x)
  {
    if (is_full()) {
      base_.destroy(base_.read_);
      base_.construct(base_.read_, std::move(x));
      AdvanceRead(1);
      advance_write(2);
    }
    else {
      base_.construct(base_.write_, std::move(x));
      advance_write(1);
    }
  }

  // void push_front(value_type x)
  // {
  //   back_read();
  //   base_.construct(base_.read_, std::move(x));
  // }

  // NOTE Don't follow the STL stype: pop_xx() API don't return poped element
  // we is going to return it that simplify the opertion:
  // auto top = q.top(); q.pop(); ==> auto top = q.pop();
  // value_type pop_back() 
  // {
  //   auto tmp = std::move(back());
  //   back_write();
  //   // this->destroy(base_.write_);
  //   return tmp;
  // }

  value_type pop_front()
  {
    auto tmp = std::move(*GetReadBegin());
    // no need to destroy element explicitly
    // this->destroy(base_.read);
    AdvanceRead();
    return tmp;
  }

  const_pointer GetReadBegin() const noexcept { return base_.read_; }


  size_type readable() const noexcept
  {
    return (base_.read_ <= base_.write_) ?
            base_.write_ - base_.read_ :
            base_.write_ - base_.begin_ + base_.end_ - base_.read_;
  }

  size_type writeable() const noexcept { return max_size() - readable(); }

  size_type max_size() const noexcept { return base_.end_ - base_.begin_ - 1; }

  void AdvanceRead(const size_type n=1)
  {
    const size_type diff = base_.end_ - base_.read_;
    base_.read_ = (diff <= n) ? base_.begin_ + diff - n : base_.read_ + n;
  }

#ifdef CIRCULAR_BUFFER_DEBUG
  void print_memory_layout() const noexcept
  {
    printf("begin(%p) | ", base_.begin_);
  
    if (base_.read_ >= base_.write_) {
      printf("[%ld elements] | write(%p) | ", (base_.write_ - base_.begin_) , base_.write_);
      printf("[hole] | read(%p) | [%lu element] | ", base_.read_, (base_.end_ - base_.read_));
    }
    else {
      printf("[%lu elements] | read(%p) | ", (base_.read_ - base_.begin_), base_.read_);
      printf("[hole] | write(%p) | [%lu element] | ", base_.write_, (base_.end_ - base_.write_));
    }
    printf("end(%p)\n", base_.end_);

  }
#endif 

private:

  bool is_full() const noexcept 
  { 
    
    return base_.read_ - 1 == base_.write_ || 
           (base_.read_ == base_.begin_ && 
            base_.write_ == base_.end_ - 1);
  }

  void advance_write(const size_type n=1) 
  {
    const size_type diff = base_.end_ - base_.write_;
    base_.write_ = (diff <= n) ? base_.begin_ + diff - n : base_.write_ + n;
  }

  detail::CircularBufferBase<T, Alloc> base_;
};


} // namespace algo_util
} // namespace kanon

#endif // KANON_ALGO_CIRCULAR_BUFFER_H
