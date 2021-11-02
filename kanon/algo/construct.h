#ifndef KANON_ALGO_CONSTRUCT_H
#define KANON_ALGO_CONSTRUCT_H

#include <utility> // std::forward
#include <type_traits> // std::is_trivially_destructible
#include <iterator> // std::iterator_traits

namespace kanon {
namespace algo_util {

template<typename T, typename... Args>
inline void construct(T* ptr, Args&&... args) {
  ::new(ptr) T{ std::forward<Args>(args)... };
}

namespace detail {

// Here I use "tag dispatching" to select function,
// FIXME or using class specilization is better?
template<typename T>
inline void destroySingle(T* ptr, std::false_type) {
  ptr->~T();
}

template<typename T>
inline void destroySingle(T* ptr, std::true_type) {
  // do nothing
}

} // namespace detail 

template<typename T> 
inline void destroy(T* ptr) {
  detail::destroySingle(
    ptr, 
    typename std::is_trivially_destructible<T>::type{});
}

namespace detail {

template<typename FI>
inline void destroyRange(FI first, FI last, std::false_type) {
  for (; first != last; ++first) {
    destroy(addressof(*first));
  }
}

template<typename FI>
inline void destroyRange(FI first, FI last, std::true_type) {
  // do something
}
} // namespace detail

template<typename FI> 
inline void destroy(FI first, FI last) {
  typedef typename std::iterator_traits<FI>::value_type ValueType;
  detail::destroyRange(
    std::move(first), std::move(last),
    typename std::is_trivially_destructible<ValueType>::type{});
}

} // namespace algo_util
} // namespace kanon

#endif // KANON_ALGO_CONSTRUCT_H