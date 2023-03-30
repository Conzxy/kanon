#ifndef KANON_UTIL_MACRO_H
#define KANON_UTIL_MACRO_H

#include "kanon/util/platform_macro.h"
#include "kanon/util/compiler_macro.h"

#define KANON_UNUSED(var) (void)var
#define KANON_RETHROW     throw

/* In windows, you add /Zc:__cplusplus option to MSVC,
   the __cplusplus >= 201402L at least(i.e. c++14) */
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
#define KANON_NOEXCEPT       noexcept
#define KANON_NOEXCEPT_OP(x) noexcept(x)
#define KANON_OVERRIDE       override
#else
#define KANON_NOEXCEPT throw()
#define KANON_NOEXCEPT_OP(x)
#define KANON_OVERRIDE
#endif

#if defined(__GNUC__) || defined(__clang__)
#define KANON_ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER) /* && !defined(__clang__) */
#define KANON_ALWAYS_INLINE __forceinline
#else
#define KANON_ALWAYS_INLINE
#endif // !defined(__GUNC__) || defined(__clang__)

#define KANON_INLINE inline KANON_ALWAYS_INLINE

#ifdef CXX_STANDARD_14
#define KANON_CONSTEXPR constexpr
#else
#define KANON_CONSTEXPR KANON_INLINE
#endif

#define KANON_RETHROW      throw
#define KANON_FUNCTION_TRY try:
#define KANON_CATCH        catch
#define KANON_CATCH_ALL    catch (...)

#define BZERO(dst, num) ::memset(dst, 0, num)

#define IMPORT_NAMESPACE(space) using namespace space

#define KANON_ASSERT(cond, msg) assert((cond) && (msg))

#if defined(__GUNC__) || defined(__clang__)
#define KANON_LIKELY(cond)   KANON_BUILTIN_EXPECT(!!(cond), 1)
#define KANON_UNLIKELY(cond) KANON_BUILTIN_EXPECT(!!(cond), 0)
#else
#define KANON_LIKELY(cond)   (cond)
#define KANON_UNLIKELY(cond) (cond)
#endif

#if defined(__GUNC__) || defined(__clang__)
#define KANON_TLS __thread
#elif defined(_MSC_VER)
#define KANON_TLS __declspec(thread)
#else
#ifdef CXX_STANDARD_11
#define KANON_TLS thread_local
#else
#define KANON_TLS
#endif
#endif

#endif // KANON_UTIL_MACRO_H
