//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Host/Platform.h"
#if defined(__linux__)
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#elif defined(_WIN32)
#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#endif

#include <sstream>

namespace {

int sLogLevel;
bool sColorsEnabled;
// stderr is handled a bit differently on Windows, especially when running
// under powershell. We can simply use stdout for log output.
#if defined(_WIN32)
FILE *sOutputStream = stdout;
#else
FILE *sOutputStream = stderr;
#endif
}

namespace ds2 {

uint32_t GetLogLevel() { return sLogLevel; }

void SetLogLevel(uint32_t level) { sLogLevel = level; }

void SetLogColorsEnabled(bool enabled) { sColorsEnabled = enabled; }

void SetLogOutputStream(FILE *stream) { sOutputStream = stream; }

void vLog(int level, char const *classname, char const *funcname,
          char const *format, va_list ap) {
  if (sLogLevel > level)
    return;

  std::stringstream ss;

  std::vector<char> buffer;
  size_t required_bytes = 128;

  do {
    va_list ap_copy;
    va_copy(ap_copy, ap);
    buffer.resize(required_bytes + 1);
    required_bytes =
        ds2_vsnprintf(buffer.data(), buffer.size(), format, ap_copy);
    va_end(ap_copy);
  } while (required_bytes >= buffer.size());

  ss << '[' << Host::Platform::GetCurrentProcessId() << ']';

  if (classname != nullptr) {
    ss << '[' << classname << "::" << funcname << ']';
  } else {
    ss << '[' << funcname << ']';
  }

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
  default:
    break;
  }

  ss << ' ';

  if (color != nullptr && sColorsEnabled) {
    ss << color << label << "\x1b[m" << ':';
  } else if (label != nullptr) {
    ss << label << ':';
  }

  ss << ' ' << buffer.data() << std::endl;

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
