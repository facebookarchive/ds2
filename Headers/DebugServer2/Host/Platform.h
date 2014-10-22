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

#include "DebugServer2/Host/Base.h"

#if defined(_WIN32)
#include "DebugServer2/Host/Windows/Platform.h"
#elif defined(__linux__)
#include "DebugServer2/Host/Linux/Platform.h"
#elif defined(__FreeBSD__)
#include "DebugServer2/Host/FreeBSD/Platform.h"
#elif defined(__NetBSD__)
#include "DebugServer2/Host/NetBSD/Platform.h"
#elif defined(__OpenBSD__)
#include "DebugServer2/Host/OpenBSD/Platform.h"
#elif defined(__QNX__)
#include "DebugServer2/Host/QNX/Platform.h"
#elif defined(sun)
#include "DebugServer2/Host/Solaris/Platform.h"
#else
#error "Your platform is not supported."
#endif

namespace ds2 {
namespace Host {

#if defined(_WIN32)
using Windows::Platform;
#elif defined(__linux__)
using Linux::Platform;
#elif defined(__FreeBSD__)
using FreeBSD::Platform;
#elif defined(__NetBSD__)
using NetBSD::Platform;
#elif defined(__OpenBSD__)
using OpenBSD::Platform;
#elif defined(__QNX__)
using QNX::Platform;
#elif defined(sun)
using Solaris::Platform;
#else
#error "Your platform is not supported."
#endif
}
}

#endif // !__DebugServer2_Host_Platform_h
