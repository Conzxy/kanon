#ifndef KANON_BOUNDED_QUEUE_H
#define KANON_BOUNDED_QUEUE_H

#include "circular_buffer.h"

namespace kanon {


template<typename T, typename Base=algo_util::CircularBuffer<T>>
class BoundedQueue {
public:
  typedef typename Base::size_type size_type;
  typedef typename Base::value_type value_type;
  typedef typename Base::reference reference;
  typedef typename Base::const_reference const_reference;
  typedef typename Base::pointer pointer;
  typedef typename Base::const_pointer const_pointer;
  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;

  explicit BoundedQueue(size_t n)
    : buffer_ { n }
  { }
  
  // don't provide push(T const&) interface
  // because this also handle that case
  template<typename ...Args>
  void push(Args&&... args) {
    buffer_.emplace(std::forward<Args>(args)...);
  }

  value_type top() noexcept {
    return *buffer_.GetReadBegin();
  }
  
  // @note
  // Difference from pop() of std::queue, 
  // pop also return the top which has removed recently
  // So, you no need to do:
  // auto top = top();
  // pop();
  value_type pop() {
    auto front = *std::move(buffer_.GetReadBegin());
    buffer_.AdvanceRead(1);
    return front;
  }
  
  size_type size() noexcept {
    return buffer_.readable();
  }

  size_type max_size() noexcept {
    return buffer_.max_size();
  }

  Base& base() noexcept {
    return buffer_;
  }
private:
  Base buffer_;  
};

} // namespace kanon

#endif // KANON_BOUNDED_QUEUE_H
