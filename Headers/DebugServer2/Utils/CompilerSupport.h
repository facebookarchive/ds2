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

#include "DebugServer2/Base.h"

#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#if defined(PLATFORM_MINGW)
// MinGW uses printf wrappers to provide standard format string behavior on
// Windows. We need to use __MINGW_PRINTF_FORMAT as GCC assumes MS style printf
// arguments with `__format__(printf, ...)`.
#define DS2_ATTRIBUTE_PRINTF(FORMAT, START)                                    \
  __attribute__((__format__(__MINGW_PRINTF_FORMAT, FORMAT, START)))
#else
#define DS2_ATTRIBUTE_PRINTF(FORMAT, START)                                    \
  __attribute__((__format__(printf, FORMAT, START)))
#endif
#else
#define DS2_ATTRIBUTE_PRINTF(FORMAT, START)
#endif

#if defined(COMPILER_GCC)
#if __GNUC__ > 6
#define DS2_FALLTHROUGH [[fallthrough]]
#else
#define DS2_FALLTHROUGH
#endif
#elif defined(COMPILER_CLANG)
#define DS2_FALLTHROUGH [[clang::fallthrough]]
#else
#define DS2_FALLTHROUGH
#endif

#if defined(COMPILER_MSVC)
#define DS2_ATTRIBUTE_PACKED "DS2_ATTRIBUTE_PACKED not implemented on MSVC"
#elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define DS2_ATTRIBUTE_PACKED __attribute__((__packed__))
#endif

#if defined(COMPILER_MSVC)
#define DS2_ATTRIBUTE_ALIGNED "DS2_ATTRIBUTE_ALIGNED not implemented on MSVC"
#elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define DS2_ATTRIBUTE_ALIGNED(SIZE) __attribute__((__aligned__(SIZE)))
#endif

#if defined(COMPILER_MSVC)
#define DS2_UNREACHABLE() __assume(0)
#elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define DS2_UNREACHABLE() __builtin_unreachable()
#else
#include <cstdlib>
#define DS2_UNREACHABLE() abort()
#endif
