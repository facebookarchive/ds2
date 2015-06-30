//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_Platform_h
#define __DebugServer2_Host_Platform_h

#if defined(_WIN32)
#include "DebugServer2/Host/Windows/Platform.h"
#elif defined(__linux__)
#include "DebugServer2/Host/Linux/Platform.h"
#else
#error "Target not supported."
#endif

namespace ds2 {
namespace Host {

#if defined(_WIN32)
using Windows::Platform;
#elif defined(__linux__)
using Linux::Platform;
#else
#error "Target not supported."
#endif
}
}

#endif // !__DebugServer2_Host_Platform_h
