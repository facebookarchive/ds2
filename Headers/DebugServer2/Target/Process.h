// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/Base.h"

#if defined(OS_LINUX)
#include "DebugServer2/Target/Linux/Process.h"
#elif defined(OS_WIN32)
#include "DebugServer2/Target/Windows/Process.h"
#elif defined(OS_FREEBSD)
#include "DebugServer2/Target/FreeBSD/Process.h"
#elif defined(OS_DARWIN)
#include "DebugServer2/Target/Darwin/Process.h"
#else
#error "Target not supported."
#endif

namespace ds2 {
namespace Target {

#if defined(OS_LINUX)
using Linux::Process;
#elif defined(OS_WIN32)
using Windows::Process;
#elif defined(OS_FREEBSD)
using FreeBSD::Process;
#elif defined(OS_DARWIN)
using Darwin::Process;
#else
#error "Target not supported."
#endif
} // namespace Target
} // namespace ds2
