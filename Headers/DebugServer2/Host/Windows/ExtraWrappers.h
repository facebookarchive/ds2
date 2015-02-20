//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_Windows_ExtraWrappers_h
#define __DebugServer2_Host_Windows_ExtraWrappers_h

#include "DebugServer2/Base.h"

#include <cstdarg>
#include <cstdio>

// MSVC does not have snprintf, and has a vsnprintf that does not have the same
// semantics as the linux one, which means we have to provide wrappers for
// both.

static inline int ds2_vsnprintf(char *str, size_t size, char const *format,
                                va_list ap) {
  int res;
  res = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
  if (res == -1)
    res = _vscprintf(format, ap);
  return res;
}

static inline int ds2_snprintf(char *str, size_t size, char const *format,
                               ...) {
  int res;
  va_list ap;
  va_start(ap, format);
  res = ds2_vsnprintf(str, size, format, ap);
  va_end(ap);
  return res;
}

#endif // !__DebugServer2_Host_Windows_ExtraWrappers_h
