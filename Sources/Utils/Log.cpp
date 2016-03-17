//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/CompilerSupport.h"
#include "DebugServer2/Utils/String.h"

#if defined(ENABLE_LOGCAT)
#include <android/log.h>
#endif
#include <sstream>

namespace {

int sLogLevel;
bool sColorsEnabled = false;
// stderr is handled a bit differently on Windows, especially when running
// under powershell. We can simply use stdout for log output.
#if defined(OS_WIN32)
FILE *sOutputStream = stdout;
#else
FILE *sOutputStream = stderr;
#endif
}

#if defined(ENABLE_LOGCAT)
static void androidLogcat(int level, char const *functag, char const *message) {
  android_LogPriority androidLevel;

  switch (level) {
#define CONVERT_LEVEL(LEVEL, ANDROID_LEVEL)                                    \
  case LEVEL:                                                                  \
    androidLevel = ANDROID_LEVEL;                                              \
    break;
    CONVERT_LEVEL(ds2::kLogLevelPacket, ANDROID_LOG_VERBOSE)
    CONVERT_LEVEL(ds2::kLogLevelDebug, ANDROID_LOG_DEBUG)
    CONVERT_LEVEL(ds2::kLogLevelInfo, ANDROID_LOG_INFO)
    CONVERT_LEVEL(ds2::kLogLevelWarning, ANDROID_LOG_WARN)
    CONVERT_LEVEL(ds2::kLogLevelError, ANDROID_LOG_ERROR)
    CONVERT_LEVEL(ds2::kLogLevelFatal, ANDROID_LOG_FATAL)
#undef CONVERT_LEVEL
  default:
    DS2_UNREACHABLE();
  }

  std::stringstream ss(message);
  std::string line;
  while (std::getline(ss, line, '\n'))
    __android_log_print(androidLevel, "ds2", "[%s] %s", functag, line.c_str());
}
#endif

namespace ds2 {

uint32_t GetLogLevel() { return sLogLevel; }

void SetLogLevel(uint32_t level) { sLogLevel = level; }

void SetLogColorsEnabled(bool enabled) { sColorsEnabled = enabled; }

void SetLogOutputStream(FILE *stream) { sOutputStream = stream; }

void vLog(int level, char const *classname, char const *funcname,
          char const *format, va_list ap) {
  std::stringstream ss;

  std::vector<char> buffer;
  size_t required_bytes = 128;

  do {
    va_list ap_copy;
    va_copy(ap_copy, ap);
    buffer.resize(required_bytes + 1);
    required_bytes =
        ds2::Utils::VSNPrintf(buffer.data(), buffer.size(), format, ap_copy);
    va_end(ap_copy);
  } while (required_bytes >= buffer.size());

  std::stringstream functag;
  if (classname != nullptr)
    functag << classname << "::";
  functag << funcname;

#if defined(ENABLE_LOGCAT)
  if (level >= sLogLevel || level >= kLogLevelDebug) {
    // If we're on Android pollute logcat as well.
    androidLogcat(level, functag.str().c_str(), buffer.data());
  }
#endif

  if (level < sLogLevel)
    return;

  ss << '[' << Host::Platform::GetCurrentProcessId() << ']';
  ss << '[' << functag.str() << ']';

  char const *color = nullptr;
  char const *label = nullptr;

  switch (level) {
  case kLogLevelFatal:
    color = "\x1b[1;31m";
    label = "FATAL  ";
    break;
  case kLogLevelError:
    color = "\x1b[1;31m";
    label = "ERROR  ";
    break;
  case kLogLevelWarning:
    color = "\x1b[1;33m";
    label = "WARNING";
    break;
  case kLogLevelInfo:
    color = "\x1b[1;32m";
    label = "INFO   ";
    break;
  case kLogLevelDebug:
    color = "\x1b[1;36m";
    label = "DEBUG  ";
    break;
  case kLogLevelPacket:
    color = "\x1b[0;35m";
    label = "PACKET  ";
    break;
  default:
    DS2_UNREACHABLE();
  }

  ss << ' ';

  if (color != nullptr && sColorsEnabled) {
    ss << color << label << "\x1b[m" << ':';
  } else if (label != nullptr) {
    ss << label << ':';
  }

  ss << ' ' << buffer.data() << std::endl;

#if defined(OS_WIN32)
  if (level >= sLogLevel || level >= kLogLevelDebug) {
    // If we're on Windows, pollute dbgview as well.
    OutputDebugStringA(ss.str().c_str());
  }
#endif

  fputs(ss.str().c_str(), sOutputStream);
  fflush(sOutputStream);

  if (level == kLogLevelFatal)
    abort();
}

void Log(int level, char const *classname, char const *funcname,
         char const *format, ...) {
  va_list ap;

  va_start(ap, format);
  vLog(level, classname, funcname, format, ap);
  va_end(ap);
}
}
