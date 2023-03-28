#ifndef KANON_UTIL_PLATFORM_MACRO_H__
#define KANON_UTIL_PLATFORM_MACRO_H__

#ifdef __linux__
#define KANON_ON_LINUX 1
#endif

#if defined(_WIN32)
#define KANON_ON_WIN 1
#endif

#if (defined(__unix__) || defined(__linux__)) && !defined(KANON_TEST_THREAD)
#define KANON_ON_UNIX 1
#endif

#define _XKEYCHECK_H
#ifndef _WINSOCKAPI_
// #define _WINSOCKAPI_
#endif

#endif