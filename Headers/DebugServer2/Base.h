//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

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
#include <iostream>
#include <type_traits>
#include <memory>
#include <utility>

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

#if defined(OS_LINUX) || defined(OS_FREEBSD) || defined(OS_DARWIN)
#define OS_POSIX
#endif

#if defined(OS_LINUX)
#if defined(__TIZEN__)
#define PLATFORM_TIZEN
#elif defined(__ANDROID__)
#define PLATFORM_ANDROID
#endif
#elif defined(OS_WIN32)
#if defined(__MINGW32__)
#define PLATFORM_MINGW
#endif
#endif

#if defined(__arm__) || defined(_M_ARM)
#define ARCH_ARM
#define BITSIZE_32
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARCH_ARM64
#define BITSIZE_64
#elif defined(__i386__) || defined(_M_IX86)
#define ARCH_X86
#define BITSIZE_32
#elif defined(__x86_64__) || defined(_M_AMD64)
#define ARCH_X86_64
#define BITSIZE_64
#else
#error "Architecture not supported."
#endif

#if defined(COMPILER_MSVC)
#define ENDIAN_LITTLE
#else
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ENDIAN_LITTLE
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ENDIAN_BIG
#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
#define ENDIAN_MIDDLE
#else
#error "Unknown endianness."
#endif
#endif

// We use this weird array_sizeof implementation to get the number of elements
// in an array in two cases we care about:
//   1) simple static arrays (e.g.: int foo[10]);
//   2) more complex structures that have a static array member but use an
//      overload of operator[] to access data, and apply some transforms to the
//      index passed. This happens in CPUState functions.
//
// array_sizeof is enabled only if the argument is not a pointer (to avoid the
// classic sizeof(pointer) bug), and if the argument is a POD type (to avoid
// users doing array_sizeof(my_std_vector) and such. The idea is that if a type
// overloads operator[] and is a POD, the underlying storage is probably in the
// structure, and not held as a reference or a pointer.
template <typename T>
typename std::enable_if<
    !std::is_pointer<T>::value && std::is_pod<T>::value,
    size_t>::type static inline constexpr array_sizeof(T const &array) {
  return sizeof(array) / (reinterpret_cast<uintptr_t>(&array[1]) -
                          reinterpret_cast<uintptr_t>(&array[0]));
}

namespace ds2 {

// We don't use C++14 (yet).
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// This thing allows classes with protected constructors to call
// make_protected_unique to construct a unique_ptr cleanly (which calls
// make_unique internally). Without this, classes with protected constructors
// cannot be make_unique'd.
template <typename T> struct make_unique_enabler {
  struct make_unique_enabler_helper : public T {
    template <typename... Args>
    make_unique_enabler_helper(Args... args) : T(std::forward<Args>(args)...) {}
  };

  template <typename... Args>
  static std::unique_ptr<T> make_protected_unique(Args... args) {
    return ds2::make_unique<make_unique_enabler_helper>(
        std::forward<Args>(args)...);
  }
};
} // namespace ds2
