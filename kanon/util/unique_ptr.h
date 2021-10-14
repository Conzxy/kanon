#ifndef KANON_UTIL_UNIQUE_PTR_H
#define KANON_UTIL_UNIQUE_PTR_H

#include <memory>
#include "kanon/util/macro.h"

namespace kanon {
	
#if __cplusplus < 201402L && __cplusplus >= 201103L
template<typename T, typename... Args>
inline std::unique_ptr<T> make_unique(Args&&... args) {
	return std::unique_ptr<T>{ new T{ std::forward<Args>(args)... } };
}
#elif __cplusplus >= 201402L
template<typename T, typename ...Args>
inline auto make_unique(Args&&... args) 
	-> decltype(std::make_unique<T>(new T{ std::forward<Args>(args)... })) {
	return std::make_unique<T>(new T{ std::forward<Args>(args)... });
}
#endif

// compatible with smart_pointer and raw pointer
template<typename T>
inline T* getPointer(std::unique_ptr<T>& ptr) KANON_NOEXCEPT {
	return ptr.get();
}

template<typename T>
inline T* getPointer(std::shared_ptr<T>&  ptr) KANON_NOEXCEPT {
	return ptr.get();
}

template<typename T>
inline T* getPointer(T* ptr) KANON_NOEXCEPT {
	return ptr;
}

} // namespace kanon

#endif
