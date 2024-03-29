#ifndef KANON_ALGO_CONSTRUCT_H
#define KANON_ALGO_CONSTRUCT_H

#include <utility>     // std::forward
#include <type_traits> // std::is_trivially_destructible
#include <iterator>    // std::iterator_traits

#include "kanon/util/macro.h"

namespace kanon {
namespace algo_util {

template <typename T, typename... Args>
KANON_INLINE void construct(T *ptr, Args &&...args)
{
  ::new (ptr) T{std::forward<Args>(args)...};
}

namespace detail {

// Here I use "tag dispatching" to select function,
// or using class specilization is better?
template <typename T>
KANON_INLINE void destroySingle(T *ptr, std::false_type)
{
  ptr->~T();
}

template <typename T>
KANON_INLINE void destroySingle(T *ptr, std::true_type)
{
  // do nothing
  KANON_UNUSED(ptr);
}

} // namespace detail

template <typename T>
KANON_INLINE void destroy(T *ptr)
{
  detail::destroySingle(ptr,
                        typename std::is_trivially_destructible<T>::type{});
}

namespace detail {

template <typename FI>
KANON_INLINE void destroyRange(FI first, FI last, std::false_type)
{
  for (; first != last; ++first) {
    destroy(addressof(*first));
  }
}

template <typename FI>
KANON_INLINE void destroyRange(FI first, FI last, std::true_type)
{
  // do something
  KANON_UNUSED(first);
  KANON_UNUSED(last);
}
} // namespace detail

template <typename FI>
KANON_INLINE void destroy(FI first, FI last)
{
  typedef typename std::iterator_traits<FI>::value_type ValueType;
  detail::destroyRange(
      std::move(first), std::move(last),
      typename std::is_trivially_destructible<ValueType>::type{});
}

} // namespace algo_util
} // namespace kanon

#endif // KANON_ALGO_CONSTRUCT_H
