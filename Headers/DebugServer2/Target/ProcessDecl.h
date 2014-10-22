//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_ProcessDecl_h
#define __DebugServer2_Target_ProcessDecl_h

#include "DebugServer2/Types.h"

#define __FORWARD_DECLARE(TARGET, NAME)                                        \
  namespace TARGET {                                                           \
  class NAME;                                                                  \
  }                                                                            \
  using TARGET::NAME;

namespace ds2 {
namespace Target {

#if defined(__linux__)
__FORWARD_DECLARE(Linux, Process);
__FORWARD_DECLARE(Linux, Thread);
#elif defined(_WIN32)
__FORWARD_DECLARE(Windows, Process);
__FORWARD_DECLARE(Windows, Thread);
#else
#error "Target not supported."
#endif
}
}

#undef __FORWARD_DECLARE

#endif // !__DebugServer2_Target_ProcessDecl_h
