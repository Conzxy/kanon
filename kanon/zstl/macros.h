#ifndef _ZSTL_MACROS_H_
#define _ZSTL_MACROS_H_

#if __cplusplus >= 201103L
#  ifndef CXX_STANDARD_11
#    define CXX_STANDARD_11
#  endif
#endif

#ifndef CXX_STANDARD_11
#  ifndef CXX_STANDARD_98
#    define CXX_STANDARD_98 // this is maybe replaced by !CXX_STANDARD_11
#  endif
#endif

#if __cplusplus >= 201402L
#  ifndef CXX_STANDARD_14
#    define CXX_STANDARD_14
#  endif
#endif

#if __cplusplus >= 201703L
#  ifndef CXX_STANDARD_17
#    define CXX_STANDARD_17
#  endif
#endif

#ifndef STD_FORWARD
#  define STD_FORWARD(Args, args) std::forward<Args>(args)
#endif

#ifdef CXX_STANDARD_14
#  define ZSTL_CONSTEXPR constexpr
#else
#  define ZSTL_CONSTEXPR KANON_INLINE
#endif

#endif
