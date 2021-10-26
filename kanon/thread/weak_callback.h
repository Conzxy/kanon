#ifndef KANON_WEAK_CALLBACK_H
#define KANON_WEAK_CALLBACK_H

#include <bits/node_handle.h>
#include <ctime>
#include <iterator>
#include <memory>
#include <functional> 
namespace kanon {

// @note
// I don't think this is great wrapper:
// member function wrapper too many( &, &&, noexcept, const, volatile i.e. 48 types)
// if std::is_member_pointer<> provide traits type alias, it's great(my TinySTL::Is_member_pointer<> can provide return and paraemter for 48 types)
// 
template<typename R, typename T, typename ...Args>
class WeakCallback
{
public:
	constexpr WeakCallback(
			std::weak_ptr<T> const& ptr,
			std::function<R(T*, Args...)> const& fun)
		: ptr_(ptr), fun_(fun)
	{ }

	constexpr R operator()(Args... args) const
	{
		auto sp = ptr_.lock();
		if(sp)
			fun_(sp.get(), args...);
	}
private:
	std::weak_ptr<T> ptr_;
	std::function<R(T*, Args...)> fun_;
	
};

// factory
#define MAKEWEAKCALLBACK(qualifier) \
template<typename R, typename T, typename ...Args> \
constexpr WeakCallback<R, T, Args...> makeWeakCallback( \
		std::shared_ptr<T> const& ptr, \
		R(T::*fun)(Args...) qualifier) \
{ \
	return WeakCallback<R, T, Args...>(ptr, fun); \
}

// total 24 types
// not including c-stype variadic parametes
MAKEWEAKCALLBACK()
MAKEWEAKCALLBACK(const)
MAKEWEAKCALLBACK(volatile)
MAKEWEAKCALLBACK(const volatile)
MAKEWEAKCALLBACK(&)
MAKEWEAKCALLBACK(const &)
MAKEWEAKCALLBACK(volatile &)
MAKEWEAKCALLBACK(const volatile &)
MAKEWEAKCALLBACK(&&)
MAKEWEAKCALLBACK(const &&)
MAKEWEAKCALLBACK(volatile &&)
MAKEWEAKCALLBACK(const volatile &&)
#if __cpluscplus >= 201703L
MAKEWEAKCALLBACK(const noexcept)
MAKEWEAKCALLBACK(noexcept)
MAKEWEAKCALLBACK(volatile noexcept)
MAKEWEAKCALLBACK(const volatile noexcept)
MAKEWEAKCALLBACK(const & noexcept)
MAKEWEAKCALLBACK(& noexcept)
MAKEWEAKCALLBACK(volatile & noexcept)
MAKEWEAKCALLBACK(const volatile & noexcept)
MAKEWEAKCALLBACK(const && noexcept)
MAKEWEAKCALLBACK(&& noexcept)
MAKEWEAKCALLBACK(volatile && noexcept)
MAKEWEAKCALLBACK(const volatile && noexcept)
#endif // __cpluscplus >= 201703L
} // namespace kanon


#endif // KANON_WEAK_CALLBACK_H
