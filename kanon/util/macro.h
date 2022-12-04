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

#ifdef __linux__
#define KANON_ON_LINUX 1
#endif

#if defined(_WIN32)
#define KANON_ON_WIN 1
#endif

#if (defined(__unix__) || defined(__linux__)) && !defined(KANON_TEST_THREAD)
#define KANON_ON_UNIX 1
#endif

#endif // KANON_UTIL_MACRO_H
