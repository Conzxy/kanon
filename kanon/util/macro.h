#ifndef KANON_UTIL_MACRO_H
#define KANON_UTIL_MACRO_H

#define KANON_UNUSED(var) (void)var
#define KANON_RETHROW throw

#if __cplusplus >= 201103L
#define KANON_NOEXCEPT noexcept
#else
#define KANON_NOEXCEPT throw()
#endif


#endif // KANON_UTIL_MACRO_H