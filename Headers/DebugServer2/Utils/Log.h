//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Utils_Log_h
#define __DebugServer2_Utils_Log_h

#include "DebugServer2/Base.h"
#include "DebugServer2/Utils/CompilerSupport.h"

#include <cinttypes>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace ds2 {

//
// Log Level
//
enum {
  kLogLevelPacket,
  kLogLevelDebug,
  kLogLevelInfo,
  kLogLevelWarning,
  kLogLevelError,
  kLogLevelFatal,
};

uint32_t GetLogLevel();
void SetLogLevel(uint32_t level);
void SetLogColorsEnabled(bool enabled);
void SetLogOutputStream(FILE *stream);

void Log(int level, char const *classname, char const *funcname,
         char const *format, ...) DS2_ATTRIBUTE_PRINTF(4, 5);

void Print(char const *format, ...);

#if defined(__DS2_LOG_CLASS_NAME__)
#define DS2LOG(LVL, ...)                                                       \
  ds2::Log(ds2::kLogLevel##LVL, __DS2_LOG_CLASS_NAME__, __FUNCTION__,          \
           __VA_ARGS__)
#else
#define DS2LOG(LVL, ...)                                                       \
  ds2::Log(ds2::kLogLevel##LVL, nullptr, __FUNCTION__, __VA_ARGS__)
#endif

// Provide usable 64-bit integer format strings on Windows.
// This is required because Microsoft's implementation of the C language
// doesn't support "%llu"; "%I64u" needs to be used instead. By defining these
// here, we make sure that the rest of the project can use these format strings
// without worrying about the underlying platform.
#if defined(OS_WIN32)
#if defined(PRIu64)
#undef PRIu64
#endif
#define PRIu64 "I64u"

#if defined(PRIi64)
#undef PRIi64
#endif
#define PRIi64 "I64i"

#if defined(PRId64)
#undef PRId64
#endif
#define PRId64 "I64d"

#if defined(PRIx64)
#undef PRIx64
#endif
#define PRIx64 "I64x"
#endif

#if !defined(NDEBUG)
#define DS2ASSERT(COND)                                                        \
  do {                                                                         \
    if (!(COND)) {                                                             \
      DS2LOG(Fatal, "assertion `%s' failed at %s:%d", #COND, __FILE__,         \
             __LINE__);                                                        \
      DS2_UNREACHABLE();                                                       \
    }                                                                          \
  } while (0)
#else
#define DS2ASSERT(COND)                                                        \
  do {                                                                         \
    if (!(COND))                                                               \
      DS2_UNREACHABLE();                                                       \
  } while (0)
#endif

#if defined(__clang__) || defined(__GNUC__)
#define DS2BUG(MESSAGE, ...)                                                   \
  do {                                                                         \
    DS2LOG(Fatal, "bug at %s:%d: " MESSAGE, __FILE__, __LINE__,                \
           ##__VA_ARGS__);                                                     \
    DS2_UNREACHABLE();                                                         \
  } while (0)
#elif defined(_MSC_VER)
#define DS2BUG(MESSAGE, ...)                                                   \
  do {                                                                         \
    DS2LOG(Fatal, "bug at %s:%d: " MESSAGE, __FILE__, __LINE__, __VA_ARGS__);  \
    DS2_UNREACHABLE();                                                         \
  } while (0)
#else
#error "Compiler not supported."
#endif
}

#endif // !__DebugServer2_Utils_Log_h
