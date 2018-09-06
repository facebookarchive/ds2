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

#include "DebugServer2/Types.h"

#define __FORWARD_DECLARE(TARGET, NAME)                                        \
  namespace TARGET {                                                           \
  class NAME;                                                                  \
  }                                                                            \
  using TARGET::NAME

namespace ds2 {
namespace Target {

class ProcessBase;

#if defined(OS_LINUX)
__FORWARD_DECLARE(Linux, Process);
__FORWARD_DECLARE(Linux, Thread);
#elif defined(OS_WIN32)
__FORWARD_DECLARE(Windows, Process);
__FORWARD_DECLARE(Windows, Thread);
#elif defined(OS_FREEBSD)
__FORWARD_DECLARE(FreeBSD, Process);
__FORWARD_DECLARE(FreeBSD, Thread);
#elif defined(OS_DARWIN)
__FORWARD_DECLARE(Darwin, Process);
__FORWARD_DECLARE(Darwin, Thread);
#else
#error "Target not supported."
#endif
} // namespace Target
} // namespace ds2

#undef __FORWARD_DECLARE
