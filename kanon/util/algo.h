#ifndef KANON_UTIL_ALGO_H
#define KANON_UTIL_ALGO_H

namespace kanon {

template<typename SM, typename T>
inline bool contains(SM const& set_or_map, T const& key_or_iter) {
	return set_or_map.find(key_or_iter) != set_or_map.end();
}

// template<typename C, typename Pred>
// void erase_if(C& cont, typename C::iterator first, typename C::iterator last, Pred pred) {
// 	for (; first != last; ++first) {
// 		if (pred(*first))
// 			cont.erase(first);
// 	}
// }

}
#endif // KANON_UTIL_ALGO_H
