#pragma once
#ifndef STL_SUPPLEMENT_TYPE_TRAITS_H
#define STL_SUPPLEMENT_TYPE_TRAITS_H

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

}

#endif // STL_SUPPLEMENT_TYPE_TRAITS_H
