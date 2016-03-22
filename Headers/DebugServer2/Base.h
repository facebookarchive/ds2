//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Base_h
#define __DebugServer2_Base_h

#if defined(_WIN32)
// clang-format off
#include <winsock2.h>
#include <windef.h>
// clang-format on
#if !defined(__MINGW32__)
typedef SSIZE_T ssize_t;
#endif
#else
#include <cstdlib>
#endif

#if defined(__clang__)
#define COMPILER_CLANG
#elif defined(__GNUC__)
#define COMPILER_GCC
#elif defined(_MSC_VER)
#define COMPILER_MSVC
#else
#error "Compiler not supported."
#endif

#if defined(__linux__)
#define OS_LINUX
#elif defined(_WIN32)
#define OS_WIN32
#elif defined(__FreeBSD__)
#define OS_FREEBSD
#elif defined(__APPLE__)
#define OS_DARWIN
#else
#error "Target not supported."
#endif

#if defined(__TIZEN__)
#define PLATFORM_TIZEN
#elif defined(__ANDROID__)
#define PLATFORM_ANDROID
#endif

#if defined(__arm__) || defined(_M_ARM)
#define ARCH_ARM
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARCH_ARM64
#elif defined(__i386__) || defined(_M_IX86)
#define ARCH_X86
#elif defined(__x86_64__) || defined(_M_AMD64)
#define ARCH_X86_64
#else
#error "Architecture not supported."
#endif

#if defined(OS_WIN32)
#define ENDIAN_LITTLE
#else
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ENDIAN_LITTLE
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ENDIAN_BIG
#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
#define ENDIAN_MIDDLE
#else
#error "Unknown endianness"
#endif
#endif

template <typename TYPE, size_t SIZE>
static inline size_t array_size(TYPE const (&)[SIZE]) {
  return SIZE;
}

#endif // !__DebugServer2_Base_h
