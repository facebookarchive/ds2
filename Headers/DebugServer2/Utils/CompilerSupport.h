//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Utils_CompilerSupport_h
#define __DebugServer2_Utils_CompilerSupport_h

#include "DebugServer2/Base.h"

// For GCC we define __has_attribute and __has_builtin to 1 because we know we
// always have recent enough versions (support for C++11 for instance).

#if !defined(__has_attribute)
#if defined(COMPILER_GCC)
#define __has_attribute(ATTR) 1
#else
#define __has_attribute(ATTR) 0
#endif
#endif

#if !defined(__has_builtin)
#if defined(COMPILER_GCC)
#define __has_builtin(BUILTIN) 1
#else
#define __has_builtin(BUILTIN) 0
#endif
#endif

#if __has_attribute(format)
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

#if __has_attribute(weak)
#define DS2_ATTRIBUTE_WEAK __attribute__((__weak__))
#else
#define DS2_ATTRIBUTE_WEAK
#endif

#if __has_attribute(noreturn)
#define DS2_ATTRIBUTE_NORETURN __attribute__((__noreturn__))
#else
#define DS2_ATTRIBUTE_NORETURN
#endif

#if defined(COMPILER_MSVC)
#define DS2_UNREACHABLE() __assume(0)
#elif __has_builtin(__builtin_unreachable)
#define DS2_UNREACHABLE() __builtin_unreachable()
#else
#include <stdlib.h>
#define DS2_UNREACHABLE() abort()
#endif

#endif // !__DebugServer2_Utils_CompilerSupport_h
