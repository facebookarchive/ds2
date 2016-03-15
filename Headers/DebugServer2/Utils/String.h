//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Utils_Utils_String_h
#define __DebugServer2_Utils_Utils_String_h

#include "DebugServer2/Utils/CompilerSupport.h"

#include <cstdarg>
#include <cstdio>
#include <sstream>
#include <string>

namespace ds2 {
namespace Utils {

// Android doesn't have std::to_string, define our own implementation.
template <typename T> static inline std::string ToString(T val) {
  std::ostringstream os;
  os << val;
  return os.str();
}

// We can't add the printf attribute directly on the inline function
// definition, we have to put it on a separate function declaration.
static inline int VSNPrintf(char *str, size_t size, char const *format,
                            va_list ap) DS2_ATTRIBUTE_PRINTF(3, 0);
static inline int SNPrintf(char *str, size_t size, char const *format, ...)
    DS2_ATTRIBUTE_PRINTF(3, 4);

#if defined(OS_WIN32)
// MSVC does not have snprintf, and has a vsnprintf that does not have the same
// semantics as the linux one, which means we have to provide wrappers for
// both.
static inline int VSNPrintf(char *str, size_t size, char const *format,
                            va_list ap) {
  int res;
  res = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
  if (res == -1) {
    res = _vscprintf(format, ap);
  }
  return res;
}
#elif defined(OS_LINUX) || defined(OS_FREEBSD) || defined(OS_DARWIN)
static inline int VSNPrintf(char *str, size_t size, char const *format,
                            va_list ap) {
  return ::vsnprintf(str, size, format, ap);
}
#endif

static inline int SNPrintf(char *str, size_t size, char const *format, ...) {
  int res;
  va_list ap;
  va_start(ap, format);
  res = VSNPrintf(str, size, format, ap);
  va_end(ap);
  return res;
}

#if defined(_MSC_VER)
#pragma deprecated(sprintf, snprintf, vsnprintf)
#else
#pragma GCC poison sprintf snprintf vsnprintf
#endif
}
}

#endif // !__DebugServer2_Utils_Utils_String_h
