#ifndef KANON_UTIL_CONFIG_H
#define KANON_UTIL_CONFIG_H

#if __cplusplus < 201402L
#define ZXY_CONSTEXPR inline 
#else // 14 or over
#define ZXY_CONSTEXPR constexpr
#endif

#endif // KANON_UTIL_CONFIG_H
