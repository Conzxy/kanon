#ifndef KANON_UTIL_COMPILER_MACRO_H__
#define KANON_UTIL_COMPILER_MACRO_H__

#if defined(__GNUC__) || defined(__clang__) || defined(__GNUG__)
#define KANON_EXPORT_ATTR     __attribute__((visibility("default")))
#define KANON_IMPORT_ATTR     __attribute__((visibility("default")))
#define KANON_DEPRECATED_ATTR __attribute__((__deprecated__))
#define KANON_NO_EXPORT       __attribute__((visibility("hidden")))
#elif defined(_MSC_VER)
#define KANON_EXPORT_ATTR     __declspec(dllexport)
#define KANON_IMPORT_ATTR     __declspec(dllimport)
#define KANON_DEPRECATED_ATTR __declspec(deprecated)
#define KANON_NO_EXPORT
#endif //! defined(__GNUC__) || defined(__clang__) || defined(__GNUG__)

// For GCC,
// There is no need to specify KANON_BUILDING_DSO since KANON_CORE_API no
// change. For archive, you no need to specify KANON_STATIC_DEFINE since gcc
// will ignore it. (However, you had better specify it, readelf is different, it
// is a LOCAL symbol).
//
// For MSVC, You should specify KANON_STATID_DEFINE for using and
// building archive. Similar, you should specify KANON_BUILDIG_DSO for building
// DSO, but don't specify anything for using. i.e. using DSO is default

#ifdef KANON_LINK_SHARED
#define KANON_LINK_CORE_SHARED
#define KANON_LINK_NET_SHARED
#endif

#ifdef KANON_LINK_CORE_SHARED
#define KANON_CORE_API    KANON_IMPORT_ATTR
#define KANON_CORE_NO_API KANON_NO_EXPORT
#elif defined(KANON_BUILD_CORE_SHARED)
#define KANON_CORE_API    KANON_EXPORT_ATTR
#define KANON_CORE_NO_API KANON_NO_EXPORT
#else
#define KANON_CORE_API
#define KANON_CORE_NO_API
#endif //! KANON_LINK_CORE_SHARED

#define KANON_CORE_DEPRECATED_API    KANON_DEPRECATED_ATTR KANON_CORE_API
#define KANON_CORE_DEPRECATED        KANON_DEPRECATED_ATTR
#define KANON_CORE_DEPRECATED_NO_API KANON_DEPRECATED_ATTR KANON_CORE_NO_API

#ifdef KANON_LINK_NET_SHARED
#define KANON_NET_API    KANON_IMPORT_ATTR
#define KANON_NET_NO_API KANON_NO_EXPORT
#elif defined(KANON_BUILD_NET_SHARED)
#define KANON_NET_API    KANON_EXPORT_ATTR
#define KANON_NET_NO_API KANON_NO_EXPORT
#else
#define KANON_NET_API
#define KANON_NET_NO_API
#endif //! KANON_LINK_NET_SHARED

#define KANON_NET_DEPRECATED_API    KANON_DEPRECATED_ATTR KANON_NET_API
#define KANON_NET_DEPRECATED        KANON_DEPRECATED_ATTR
#define KANON_NET_DEPRECATED_NO_API KANON_DEPRECATED_ATTR KANON_NET_NO_API

#if defined(__GNUC__) || defined(__clang__)
#define KANON___THREAD_DEFINED 1
#else
#define KANON___THREAD_DEFINED 0
#endif //! defined(__GNUC__) || defined(__clang__)

#endif //! KANON_UTIL_COMPILER_MACRO_H__
