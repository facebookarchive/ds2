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

#include "DebugServer2/Utils/CompilerSupport.h"

#include <cstdarg>
#include <cstdio>
#include <sstream>
#include <string>

#if defined(OS_WIN32)
#include <windows.h>
#endif

#define STR_HELPER(S) #S
#define STR(S) STR_HELPER(S)

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

#if defined(OS_WIN32) && !defined(PLATFORM_MINGW)
// MSVC does not have snprintf, and has a vsnprintf that does not have the same
// semantics as the linux one, which means we have to provide wrappers for
// both.
static inline int VSNPrintf(char *str, size_t size, char const *format,
                            va_list ap) {
  int res = -1;
  if (size > 0) {
    res = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
  }
  if (res == -1) {
    res = _vscprintf(format, ap);
  }
  return res;
}
#elif defined(OS_POSIX) || (defined(OS_WIN32) && defined(PLATFORM_MINGW))
// The posix systems we support, as well as MinGW (which provides a gnu
// environment on Windows, with GNU semantics) have a sane vsnprintf. Use that.
static inline int VSNPrintf(char *str, size_t size, char const *format,
                            va_list ap) {
  return ::vsnprintf(str, size, format, ap);
}
#endif

static inline int SNPrintf(char *str, size_t size, char const *format, ...) {
  va_list ap;
  va_start(ap, format);
  int res = VSNPrintf(str, size, format, ap);
  va_end(ap);
  return res;
}

#if defined(OS_WIN32)
static inline std::wstring NarrowToWideString(std::string const &s) {
  std::vector<wchar_t> res;
  int size;

  size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
  DS2ASSERT(size != 0);
  res.resize(size);
  MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, res.data(), size);
  return res.data();
}

static inline std::string WideToNarrowString(std::wstring const &s) {
  std::vector<char> res;
  int size;

  size = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr,
                             nullptr);
  DS2ASSERT(size != 0);
  res.resize(size);
  WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, res.data(), size, nullptr,
                      nullptr);
  return res.data();
}
#endif

#if defined(COMPILER_MSVC)
#pragma deprecated(sprintf, snprintf, vsnprintf)
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#pragma GCC poison sprintf snprintf vsnprintf
#endif
} // namespace Utils
} // namespace ds2
