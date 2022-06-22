#ifndef _ZSTL_UNINITIALIZED_H_
#define _ZSTL_UNINITIALIZED_H_

#include <memory>
#include <type_traits>

#include "iterator.h"

namespace zstl {

template<typename II, typename OI>
OI UninitializedMoveIfNoexcept(II first, II last, OI output) {
  return std::uninitialized_copy(
      MAKE_MOVE_ITERATOR_IF_NOEXCEPT(first),
      MAKE_MOVE_ITERATOR_IF_NOEXCEPT(last), 
      output);
}

// After C++17
// std::uninitialized_default_construct()
// NOTE: it return void
template<typename II, typename=zstl::enable_if_t<!std::is_trivial<iter_value_t<II>>::value>>
II UninitializedDefaultConstruct(II first, II last) {
  using ValueType = iter_value_t<II>;
  
  auto old_first = first; 
  try {
    for (; first != last; ++first) {
      new(std::addressof(*first)) ValueType();
    }
  } catch (...) {
    for (; old_first != first; ++old_first) {
      old_first->~ValueType();
    }

    throw;
  }

  return last;
}

template<typename II, zstl::enable_if_t<std::is_trivial<iter_value_t<II>>::value, int> =0>
II UninitializedDefaultConstruct(II first, II last) {
  (void)first;
  (void)last;

  return last;
}

template<typename II, typename=zstl::enable_if_t<!std::is_trivial<iter_value_t<II>>::value>>
void DestroyRange(II first, II last) {
  using ValueType = iter_value_t<II>;
  for (; first != last; ++first) {
    first->~ValueType();
  }
}

template<typename II, zstl::enable_if_t<std::is_trivial<iter_value_t<II>>::value, int> =0>
void DestroyRange(II first, II last) {
  (void)first;
  (void)last;
}

} // zstl

#endif // _ZSTL_UNINITIALIZED_H_
