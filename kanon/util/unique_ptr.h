#ifndef KANON_UTIL_UNIQUE_PTR_H
#define KANON_UTIL_UNIQUE_PTR_H

#include <memory>

namespace kanon {
	
#if __cplusplus < 201402L && __cplusplus >= 201103L
template<typename T, typename... Args>
inline std::unique<T>&& make_unique(Args&&... args) {
	return std::unique<T>{ new T{ std::forward<Args>(args)... } };
}
#else
template<typename T, typename ...Args>
inline auto make_unique(Args&&... args) 
	-> decltype(std::make_unique<T>(new T{ std::forward<Args>(args)... })) {
	return std::make_unique<T>(new T{ std::forward<Args>(args)... });
}
#endif

}

#endif
