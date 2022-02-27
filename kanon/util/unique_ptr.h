#ifndef KANON_UTIL_UNIQUE_PTR_H
#define KANON_UTIL_UNIQUE_PTR_H

#include <memory>
#include <type_traits>

#include "kanon/util/macro.h"

namespace kanon {
  
#if defined(CXX_STANDARD_11) && !defined(CXX_STANDARD_14)

template<typename T>
struct Is_bounded_array : std::false_type {};

template<typename T>
struct Is_bounded_array<T[]> : std::true_type {};

template<typename T, typename... Args,
  typename std::enable_if<!std::is_array<T>::value, char>::type = 0>
inline std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<typename T,
  typename std::enable_if<Is_bounded_array<T>::value, int>::type = 0>
inline std::unique_ptr<T> make_unique(size_t num) {
  return std::unique_ptr<T>(new typename std::remove_extent<T>::type[num]);
}

#elif defined(CXX_STANDARD_14)
using std::make_unique;
#endif

// compatible with smart_pointer and raw pointer
template<typename T>
inline T* GetPointer(std::unique_ptr<T> const& ptr) noexcept {
  return ptr.get();
}

template<typename T>
inline T* GetPointer(std::shared_ptr<T> const&  ptr) noexcept {
  return ptr.get();
}

template<typename T>
inline T* GetPointer(T* const ptr) noexcept {
  return ptr;
}

template<typename T, typename F>
inline std::unique_ptr<T> down_pointer_cast(std::unique_ptr<F>& ptr)
{
#ifndef NDEBUG
  assert(ptr != nullptr && dynamic_cast<T*>(ptr.get()) != nullptr);
#endif

  return std::unique_ptr<T>(reinterpret_cast<T*>(ptr.release()));
}

} // namespace kanon

#endif
