//
// Copyright (c) 2014, Facebook, Inc.
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

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace ds2 {

//
// Log Level
//
enum {
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

#ifdef __DS2_LOG_CLASS_NAME__
#define DS2LOG(LVL, ...)                                                       \
  ds2::Log(ds2::kLogLevel##LVL, __DS2_LOG_CLASS_NAME__, __FUNCTION__,          \
           __VA_ARGS__)
#else
#define DS2LOG(LVL, ...)                                                       \
  ds2::Log(ds2::kLogLevel##LVL, nullptr, __FUNCTION__, __VA_ARGS__)
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
#define DS2ASSERT(COND) (void)0
#endif

#if defined(__clang__) || defined(__GNUC__)
#define DS2BUG(MESSAGE, ...)                                                   \
  do {                                                                         \
    DS2LOG(Fatal, "bug at %s:%d:" MESSAGE, __FILE__, __LINE__, ##__VA_ARGS__); \
    DS2_UNREACHABLE();                                                         \
  } while (0)
#elif defined(_MSC_VER)
#define DS2BUG(MESSAGE, ...)                                                   \
  do {                                                                         \
    DS2LOG(Fatal, "bug at %s:%d:" MESSAGE, __FILE__, __LINE__, __VA_ARGS__);   \
    DS2_UNREACHABLE();                                                         \
  } while (0)
#else
#error "Compiler not supported."
#endif
}

#endif // !__DebugServer2_Utils_Log_h
