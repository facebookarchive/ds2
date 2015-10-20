//
// Copyright (c) 2014, Facebook, Inc.
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

#if defined(OS_WIN32)
#include "DebugServer2/Support/Windows/Stringify.h"
#elif defined(OS_LINUX)
#include "DebugServer2/Support/POSIX/Stringify.h"
#else
#error "Target not supported."
#endif

namespace ds2 {
namespace Support {

#if defined(OS_WIN32)
using Windows::Stringify;
#elif defined(OS_LINUX)
using POSIX::Stringify;
#else
#error "Target not supported."
#endif
}
}

#endif // !__DebugServer2_Support_Stringify_h
