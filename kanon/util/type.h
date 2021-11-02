#ifndef KANON_UTIL_TYPE_H
#define KANON_UTIL_TYPE_H

#include <map>
#include <set>

namespace kanon {
	// FIXME use TinySTL::set/map instead of std::set/map
	//
	// maybe use macro is more better if not familiar with STL
	template<typename T, typename Compare = std::less<T>>
	using set = std::set<T>;

	template<typename K, typename V, typename Compare = std::less<K>>
	using map = std::map<K, V, Compare>;


} // namespace kanon

#endif // KANON_UTIL_TYPE_H
