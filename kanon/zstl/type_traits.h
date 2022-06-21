#ifndef STL_SUPPLEMENT_TYPE_TRAITS_H
#define STL_SUPPLEMENT_TYPE_TRAITS_H

#include <functional>
#include <type_traits>

#include "macros.h"

namespace zstl {

#ifdef CXX_STANDARD_17
  template<typename... Ts>
  using void_t = std::void_t<Ts...>;
#else
  template<typename... Ts>
  struct make_void { using type = void; };

  template<typename... Ts>
  using void_t = typename make_void<Ts...>::type;
#endif

// After c++14
template<bool cond, typename T=void>
using enable_if_t = typename std::enable_if<cond, T>::type;

template<bool val>
using bool_constant = std::integral_constant<bool, val>;

// After C++14
template<bool cond, typename T1, typename T2>
using conditional_t = typename std::conditional<cond, T1, T2>::type;

template<typename T>
struct type_identity : T {};

#ifdef CXX_STANDARD_17
  template<typename... Args>
  using conjunction = std::conjunction<Args...>;

  template<typename T>
  using negation = std::negation<T>;
  
  template<typename... Args>
  using disjunction = std::disjunction<Args...>;

#else
  template<typename... Args>
  struct conjunction;

  template<typename T>
  struct conjunction<T> : T {};
  
  // 短路(short-circuting)
  template<typename T, typename... Args>
  struct conjunction<T, Args...> : conditional_t<T::value, conjunction<Args...>, std::false_type> {
  };
  
  template<typename... Args>
  struct disjunction;

  template<typename T>
  struct disjunction<T> : T {};

  template<typename T, typename... Args>
  struct disjunction<T, Args...> : conditional_t<T::value, std::true_type, disjunction<Args...>> {};

  template<typename T>
  struct negation : bool_constant<!T::value> {};
#endif

#define DEFINE_HAS_NONTYPE_MEMBER(member) \
  template<typename T, typename=zstl::void_t<>> \
  struct has_nontype_member_##member : std::false_type {}; \
  template<typename T> \
  struct has_nontype_member_##member<T, zstl::void_t<decltype(&T::member)>> : std::true_type {};

}

#endif // STL_SUPPLEMENT_TYPE_TRAITS_H
