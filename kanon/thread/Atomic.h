/*
 * @version: 0.1 2021-5-28
 * @author: Conzxy
 * atomic counter
 */

#ifndef _ATOMIC_H
#define _ATOMIC_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"

#include <type_traits>
#include <stdint.h>

namespace kanon{

template<typename T>
class Atomic : noncopyable
{
public:
	static_assert(std::is_integral<T>::value, 
			"Atomic type must be integral\n");

	Atomic()
		: val_{0}
	{}

	T get() KANON_NOEXCEPT {
		return __atomic_load_n(&val_, __ATOMIC_SEQ_CST);	
	}
	
	T getAndAdd(T x) KANON_NOEXCEPT {
		return __atomic_fetch_add(&val_, x, __ATOMIC_SEQ_CST);
	}

	T addAndGet(T x) KANON_NOEXCEPT {
		return getAndAdd(x) + x;
	}

	T incrementAndGet() KANON_NOEXCEPT {
		return addAndGet(1);
	}
	
	T decrementAndGet() KANON_NOEXCEPT {
		return addAndGet(-1);
	}

	void add(T x) KANON_NOEXCEPT {
		getAndAdd(x);
	}

	void increment() KANON_NOEXCEPT {
		getAndAdd(1);
	}

	void decrement() KANON_NOEXCEPT {
		getAndAdd(-1);
	}

	T getAndSet(T x) KANON_NOEXCEPT {
		return __atomic_exchange_n(&val_, x, __ATOMIC_SEQ_CST);
	}

private:
	volatile T val_;
};

template class Atomic<int32_t>;
template class Atomic<int64_t>;

using AtomicInt32 = Atomic<int32_t>;
using AtomicInt64 = Atomic<int64_t>;

}//namespace kanon
#endif //_ATOMIC_H
