//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Support_StringifyPrivate_h
#define __DebugServer2_Support_StringifyPrivate_h

#include "DebugServer2/Utils/String.h"

#if defined(OS_DARWIN)
#define ATT_TLS __thread
#elif defined(OS_LINUX) || defined(OS_WIN32) || defined(OS_FREEBSD)
#define ATT_TLS thread_local
#else
#error "Target not supported."
#endif

#define DO_STRINGIFY(VALUE)                                                    \
  case VALUE:                                                                  \
    return #VALUE;

#define DO_DEFAULT(MESSAGE, VALUE)                                             \
  default:                                                                     \
    do {                                                                       \
      DS2LOG(Warning, MESSAGE ": %#lx", (unsigned long)VALUE);                 \
      static ATT_TLS char tmp[20];                                             \
      ds2::Utils::SNPrintf(tmp, sizeof(tmp), "%#lx", (unsigned long)VALUE);    \
      return tmp;                                                              \
    } while (0);

#endif // !__DebugServer2_Support_StringifyPrivate_h
