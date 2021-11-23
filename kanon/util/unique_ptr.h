#ifndef KANON_UTIL_UNIQUE_PTR_H
#define KANON_UTIL_UNIQUE_PTR_H

#include <memory>
#include "kanon/util/macro.h"

namespace kanon {
  
#if defined(CXX_STANDARD_11) && !defined(CXX_STANDARD_14)
namespace detail {
  template<typename T, typename... Args>
  struct MakeUnique {
    std::unique_ptr<T> operator()(Args&&... args) {
      return std::unique_ptr<T>{ new T{ std::forward<Args>(args)... } };
    }
  };

  template<typename T, typename... Args> 
  struct MakeUnique<T[], Args...> {
    std::unique_ptr<T> operator()(size_t num) {
      return std::unique_ptr<T>{ new T[num] };
    }
  };
}

template<typename T, typename... Args>
inline std::unique_ptr<T> make_unique(Args&&... args) {
  return detail::MakeUnique<T, Args...>{}(std::forward<Args>(args)...);
}

#elif defined(CXX_STANDARD_14)
using std::make_unique;
#endif

// compatible with smart_pointer and raw pointer
template<typename T>
inline T* getPointer(std::unique_ptr<T> const& ptr) KANON_NOEXCEPT {
  return ptr.get();
}

template<typename T>
inline T* getPointer(std::shared_ptr<T> const&  ptr) KANON_NOEXCEPT {
  return ptr.get();
}

template<typename T>
inline T* getPointer(T* const ptr) KANON_NOEXCEPT {
  return ptr;
}


} // namespace kanon

#endif
