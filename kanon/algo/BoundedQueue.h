#ifndef KANON_BOUNDED_QUEUE_H
#define KANON_BOUNDED_QUEUE_H

#include "kanon/algo/RingBuffer.h"

namespace kanon {

template<typename T>
class BoundedQueue {
public:
  typedef RingBuffer<T> Base;
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

  value_type top() KANON_NOEXCEPT {
    return *buffer_.peek();
  }
  
  // @note
  // Difference from pop() of std::queue, 
  // pop also return the top which has removed recently
  // So, you no need to do:
  // auto top = top();
  // pop();
  value_type pop() {
    auto front = *buffer_.peek();
    buffer_.advance(1);
    return front;
  }
  
  size_type size() KANON_NOEXCEPT {
    return buffer_.readable();
  }

  size_type maxSize() KANON_NOEXCEPT {
    return buffer_.maxSize();
  }

  Base& base() KANON_NOEXCEPT {
    return buffer_;
  }
private:
  Base buffer_;  
};

} // namespace kanon

#endif // KANON_BOUNDED_QUEUE_H
