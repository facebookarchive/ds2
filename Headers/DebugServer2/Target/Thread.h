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

#include "DebugServer2/Base.h"

#if defined(OS_LINUX)
#include "DebugServer2/Target/Linux/Thread.h"
#elif defined(OS_WIN32)
#include "DebugServer2/Target/Windows/Thread.h"
#elif defined(OS_FREEBSD)
#include "DebugServer2/Target/FreeBSD/Thread.h"
#elif defined(OS_DARWIN)
#include "DebugServer2/Target/Darwin/Thread.h"
#else
#error "Target not supported."
#endif

namespace ds2 {
namespace Target {

#if defined(OS_LINUX)
using Linux::Thread;
#elif defined(OS_WIN32)
using Windows::Thread;
#elif defined(OS_FREEBSD)
using FreeBSD::Thread;
#elif defined(OS_DARWIN)
using Darwin::Thread;
#else
#error "Target not supported."
#endif
} // namespace Target
} // namespace ds2
