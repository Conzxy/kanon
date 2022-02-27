#ifndef KANON_UTIL_MACRO_H
#define KANON_UTIL_MACRO_H

#define KANON_UNUSED(var) (void)var
#define KANON_RETHROW throw

#if __cplusplus >= 201103L
#define CXX_STANDARD_11
#endif

#if __cplusplus >= 201402L
#define CXX_STANDARD_14
#endif

#if __cplusplus >= 201703L
#define CXX_STANDARD_17
#endif

#if __cplusplus < 201103L
#define CXX_STANDARD_LESS_THAN_11
#endif

#ifdef CXX_STANDARD_11
#define noexcept noexcept
#define KANON_OVERRIDE override
#else
#define noexcept throw()
#define KANON_OVERRIDE
#endif

#ifdef CXX_STANDARD_14
#define KANON_CONSTEXPR constexpr
#else
#define KANON_CONSTEXPR inline
#endif

#define KANON_RETHROW throw
#define KANON_FUNCTION_TRY try: 
#define KANON_CATCH catch 
#define KANON_CATCH_ALL catch (...) 

#define BZERO(dst, num) \
  ::memset(dst, 0, num)

#define IMPORT_NAMESPACE(space) \
  using namespace space

#define KANON_ASSERT(cond, msg) \
  assert((cond) && (msg))

#endif // KANON_UTIL_MACRO_H
