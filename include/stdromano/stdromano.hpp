// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO)
#define __STDROMANO

/* Clang defines __GNUC__ (and clang-cl defines _MSC_VER), so it must be tested first */
#if defined(__clang__)
#define STDROMANO_CLANG
#elif defined(_MSC_VER)
#define STDROMANO_MSVC
#elif defined(__GNUC__)
#define STDROMANO_GCC
#else
#error "Unsupported compiler"
#endif /* defined(__clang__) */

#if defined(_MSC_VER)
#pragma warning(disable : 4711) /* function selected for automatic inline expansion */
#define _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif /* defined(_MSC_VER) */

#define STDROMANO_STRIFY(x) #x
#define STDROMANO_STRIFY_MACRO(m) STDROMANO_STRIFY(m)

#if !defined(STDROMANO_VERSION_MAJOR)
#define STDROMANO_VERSION_MAJOR 0
#endif /* !defined(STDROMANO_VERSION_MAJOR) */

#if !defined(STDROMANO_VERSION_MINOR)
#define STDROMANO_VERSION_MINOR 0
#endif /* !defined(STDROMANO_VERSION_MINOR) */

#if !defined(STDROMANO_VERSION_PATCH)
#define STDROMANO_VERSION_PATCH 0
#endif /* !defined(STDROMANO_VERSION_PATCH) */

#if !defined(STDROMANO_VERSION_REVISION)
#define STDROMANO_VERSION_REVISION 0
#endif /* !defined(STDROMANO_VERSION_REVISION) */

#define STDROMANO_VERSION_STR                                                                      \
    STDROMANO_STRIFY_MACRO(STDROMANO_VERSION_MAJOR)                                                \
    "." STDROMANO_STRIFY_MACRO(STDROMANO_VERSION_MINOR) "." STDROMANO_STRIFY_MACRO(                \
        STDROMANO_VERSION_PATCH) "." STDROMANO_STRIFY_MACRO(STDROMANO_VERSION_REVISION)

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>

/* Architecture detection */
/* https://github.com/cpredef/predef/blob/master/Architectures.md */
#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
#define STDROMANO_X86_64
#define STDROMANO_SIZEOF_PTR 8
#define STDROMANO_ARCH_STR "X86_64"
#elif defined(__i386__) || defined(_M_IX86)
#define STDROMANO_X86
#define STDROMANO_SIZEOF_PTR 4
#define STDROMANO_ARCH_STR "X86"
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
#define STDROMANO_AARCH64
#define STDROMANO_SIZEOF_PTR 8
#define STDROMANO_ARCH_STR "AARCH64"
#elif defined(__arm__) || defined(_M_ARM)
#define STDROMANO_AARCH32
#define STDROMANO_SIZEOF_PTR 4
#define STDROMANO_ARCH_STR "AARCH32"
#else
#error "Unsupported architecture"
#endif /* defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64) */

/* Umbrella architecture macros */
#if defined(STDROMANO_X86_64) || defined(STDROMANO_X86)
#define STDROMANO_INTEL
#elif defined(STDROMANO_AARCH64) || defined(STDROMANO_AARCH32)
#define STDROMANO_ARM
#endif /* defined(STDROMANO_X86_64) || defined(STDROMANO_X86) */

#if STDROMANO_SIZEOF_PTR == 8
#define STDROMANO_64BITS
#else
#define STDROMANO_32BITS
#endif /* STDROMANO_SIZEOF_PTR == 8 */

/* Operating system detection */
/* https://sourceforge.net/p/predef/wiki/OperatingSystems */
#if defined(_WIN32)
#define STDROMANO_WIN
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif /* !defined(WIN32_LEAN_AND_MEAN) */
#if !defined(NOMINMAX)
#define NOMINMAX
#endif /* !defined(NOMINMAX) */
#define STDROMANO_OS_STR "WIN"
#elif defined(__linux__)
#define STDROMANO_LINUX
#define STDROMANO_UNIX
#define STDROMANO_OS_STR "LINUX"
#elif defined(__APPLE__)
#define STDROMANO_APPLE
#define STDROMANO_UNIX
#define STDROMANO_OS_STR "APPLE"
#elif defined(__FreeBSD__)
#define STDROMANO_FREEBSD
#define STDROMANO_BSD
#define STDROMANO_UNIX
#define STDROMANO_OS_STR "FREEBSD"
#elif defined(__OpenBSD__)
#define STDROMANO_OPENBSD
#define STDROMANO_BSD
#define STDROMANO_UNIX
#define STDROMANO_OS_STR "OPENBSD"
#elif defined(__NetBSD__)
#define STDROMANO_NETBSD
#define STDROMANO_BSD
#define STDROMANO_UNIX
#define STDROMANO_OS_STR "NETBSD"
#elif defined(__DragonFly__)
#define STDROMANO_DRAGONFLY
#define STDROMANO_BSD
#define STDROMANO_UNIX
#define STDROMANO_OS_STR "DRAGONFLY"
#else
#error "Unsupported platform"
#endif /* defined(_WIN32) */

#define STDROMANO_PLATFORM_STR STDROMANO_OS_STR "_" STDROMANO_ARCH_STR

#if defined(STDROMANO_WIN)
#if defined(STDROMANO_MSVC)
#define STDROMANO_EXPORT __declspec(dllexport)
#define STDROMANO_IMPORT __declspec(dllimport)
#elif defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
#define STDROMANO_EXPORT __attribute__((dllexport))
#define STDROMANO_IMPORT __attribute__((dllimport))
#endif /* defined(STDROMANO_MSVC) */
#elif defined(STDROMANO_UNIX)
#define STDROMANO_EXPORT __attribute__((visibility("default")))
#define STDROMANO_IMPORT
#endif /* defined(STDROMANO_WIN) */

#if defined(STDROMANO_MSVC)
#define STDROMANO_FORCE_INLINE __forceinline
#define STDROMANO_LIB_ENTRY
#define STDROMANO_LIB_EXIT
#elif defined(STDROMANO_GCC)
#define STDROMANO_FORCE_INLINE inline __attribute__((always_inline))
#define STDROMANO_LIB_ENTRY __attribute__((constructor))
#define STDROMANO_LIB_EXIT __attribute__((destructor))
#elif defined(STDROMANO_CLANG)
#define STDROMANO_FORCE_INLINE inline __attribute__((always_inline))
#define STDROMANO_LIB_ENTRY __attribute__((constructor))
#define STDROMANO_LIB_EXIT __attribute__((destructor))
#endif /* defined(STDROMANO_MSVC) */

#if defined(STDROMANO_BUILD_SHARED)
#define STDROMANO_API STDROMANO_EXPORT
#define STDROMANO_EXPIMP_TEMPLATE
#else
#define STDROMANO_API STDROMANO_IMPORT
#define STDROMANO_EXPIMP_TEMPLATE extern
#endif /* defined(STDROMANO_BUILD_SHARED) */

#if defined __cplusplus
#define STDROMANO_CPP_ENTER                                                                        \
    extern "C"                                                                                     \
    {
#define STDROMANO_CPP_END }
#else
#define STDROMANO_CPP_ENTER
#define STDROMANO_CPP_END
#endif /* DEFINED __cplusplus */

#if !defined NULL
#define NULL (void*)0
#endif /* !defined NULL */

#if defined(STDROMANO_WIN)
#define STDROMANO_FUNCTION __FUNCTION__
#elif defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
#define STDROMANO_FUNCTION __PRETTY_FUNCTION__
#endif /* STDROMANO_WIN */

#define CONCAT_(prefix, suffix) prefix##suffix
#define CONCAT(prefix, suffix) CONCAT_(prefix, suffix)

#define STDROMANO_ASSERT(expr, message)                                                            \
    if(!(expr))                                                                                    \
    {                                                                                              \
        std::fprintf(stderr,                                                                       \
                     "Assertion failed in file %s at line %d: %s\n",                               \
                     __FILE__,                                                                     \
                     __LINE__,                                                                     \
                     message);                                                                     \
        std::abort();                                                                              \
    }

#define STDROMANO_STATIC_ASSERT(expr, message) static_assert(expr, message)
#define STDROMANO_NOT_IMPLEMENTED                                                                  \
    std::fprintf(stderr,                                                                           \
                 "Called function %s that is not implemented (%s:%d)\n",                           \
                 STDROMANO_FUNCTION,                                                               \
                 __FILE__,                                                                         \
                 __LINE__);                                                                        \
    std::exit(1)

#define STDROMANO_NON_COPYABLE(__class__)                                                          \
    __class__(const __class__&) = delete;                                                          \
    __class__& operator=(const __class__&) = delete

#define STDROMANO_NON_MOVABLE(__class__)                                                           \
    __class__(__class__&&) = delete;                                                               \
    __class__& operator=(__class__&&) = delete

#if defined(STDROMANO_MSVC)
#define STDROMANO_PACKED_STRUCT(__struct__) __pragma(pack(push, 1)) __struct__ __pragma(pack(pop))
#elif defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
#define STDROMANO_PACKED_STRUCT(__struct__) __struct__ __attribute__((__packed__))
#else
#define STDROMANO_PACKED_STRUCT(__struct__) __struct__
#endif /* defined(STDROMANO_MSVC) */

#define STDROMANO_NO_DISCARD [[nodiscard]]
#define STDROMANO_MAYBE_UNUSED [[maybe_unused]]
#define STDROMANO_UNUSED(x) ((void)x)

#if defined(STDROMANO_MSVC)
#define dump_struct(s)
#elif defined(STDROMANO_CLANG)
#define dump_struct(s) __builtin_dump_struct(s, printf)
#elif defined(STDROMANO_GCC)
#define dump_struct(s)
#endif /* defined(STDROMANO_MSVC) */

#define STDROMANO_HAS_BUILTIN(name) defined(__has_builtin) && __has_builtin(name)

#if defined(DEBUG_BUILD)
#define STDROMANO_DEBUG 1
#else
#define STDROMANO_DEBUG 0
#endif /* defined(DEBUG_BUILD) */

#define STDROMANO_NAMESPACE_BEGIN namespace stdromano {
#define STDROMANO_NAMESPACE_END }

#define DETAIL_NAMESPACE_BEGIN namespace detail {
#define DETAIL_NAMESPACE_END }

#define STDROMANO_ATEXIT_REGISTER(func, do_exit)                                                   \
    int res_##func = std::atexit(func);                                                            \
    if(res_##func != 0)                                                                            \
    {                                                                                              \
        std::fprintf(stderr, "Cannot register function \"" #func "\" in atexit");                  \
        if(do_exit)                                                                                \
            std::exit(1);                                                                          \
    }

#endif /* !defined(__STDROMANO) */
