#pragma once
#ifndef STL_SUPPLEMENT_MACROS_H
#define STL_SUPPLEMENT_MACROS_H

#if __cplusplus >= 201103L
#define CXX_STANDARD_11
#endif 

#ifndef CXX_STANDARD_11
#define CXX_STANDARD_98 // this is maybe replaced by !CXX_STANDARD_11
#endif 

#if __cplusplus >= 201402L
#define CXX_STANDARD_14
#endif

#if __cplusplus >= 201703L
#define CXX_STANDARD_17
#endif 

#define STD_FORWARD(Args, args) std::forward<Args>(args)

#ifdef CXX_STANDARD_14
#define ZSTL_CONSTEXPR constexpr
#else
#define ZSTL_CONSTEXPR inline
#endif

#endif // STL_SUPPLEMENT_MACROS_H
