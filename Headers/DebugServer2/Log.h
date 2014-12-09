//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Log_h
#define __DebugServer2_Log_h

#include "DebugServer2/Base.h"
#include "DebugServer2/CompilerSupport.h"

#include <cstdarg>
#include <cstdint>
#include <cstdio>

namespace ds2 {

//
// Log Categories
//
enum {
  kLogMain,
  kLogDebugSession,
  kLogPlatformSession,
  kLogSlaveSession,
  kLogBPManager,
  kLogProtocol,
  kLogRemote,
  kLogArchitecture,
  kLogTarget
};

//
// Log Level
//
enum {
  kLogLevelDebug,
  kLogLevelInfo,
  kLogLevelWarning,
  kLogLevelError,
  kLogLevelFatal
};

uint32_t GetLogLevel();
void SetLogLevel(uint32_t level);
void SetLogMask(uint64_t mask);
void SetLogColorsEnabled(bool enabled);
void SetLogOutputStream(FILE *stream);

void Log(int category, int level, char const *classname, char const *funcname,
         char const *format, ...) DS2_ATTRIBUTE_PRINTF(5, 6);

#ifdef __DS2_LOG_CLASS_NAME__
#define DS2LOG(CAT, LVL, ...)                                                  \
  ds2::Log(ds2::kLog##CAT, ds2::kLogLevel##LVL, __DS2_LOG_CLASS_NAME__,        \
           __FUNCTION__, __VA_ARGS__)
#else
#define DS2LOG(CAT, LVL, ...)                                                  \
  ds2::Log(ds2::kLog##CAT, ds2::kLogLevel##LVL, nullptr, __FUNCTION__,         \
           __VA_ARGS__)
#endif

#if !defined(NDEBUG)
#define DS2ASSERT(COND)                                                        \
  do {                                                                         \
    if (!(COND))                                                               \
      DS2LOG(Main, Fatal, "assertion `%s' failed at %s:%d", #COND, __FILE__,   \
             __LINE__);                                                        \
  } while (0)
#else
#define DS2ASSERT(COND) (void)0
#endif
}

#endif // !__DebugServer2_Log_h
