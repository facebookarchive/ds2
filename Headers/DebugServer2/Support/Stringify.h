//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Support_Stringify_h
#define __DebugServer2_Support_Stringify_h

#include "DebugServer2/Base.h"
#include "DebugServer2/Constants.h"
#include "DebugServer2/Types.h"

#if defined(OS_WIN32)
#include "DebugServer2/Support/Windows/Stringify.h"
#elif defined(OS_LINUX) || defined(OS_FREEBSD) || defined(OS_DARWIN)
#include "DebugServer2/Support/POSIX/Stringify.h"
#else
#error "Target not supported."
#endif

namespace ds2 {
namespace Support {

#if defined(OS_WIN32)
class Stringify : public Windows::Stringify
#elif defined(OS_LINUX) || defined(OS_FREEBSD) || defined(OS_DARWIN)
class Stringify : public POSIX::Stringify
#else
#error "Target not supported."
class Stringify
#endif
{
public:
  static char const *Error(ErrorCode error);
  static char const *StopEvent(StopInfo::Event event);
  static char const *StopReason(StopInfo::Reason reason);
};
}
}

#endif // !__DebugServer2_Support_Stringify_h
