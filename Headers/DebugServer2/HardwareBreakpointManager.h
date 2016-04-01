//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_HardwareBreakpointManager_h
#define __DebugServer2_HardwareBreakpointManager_h

#include "DebugServer2/Base.h"

#if defined(ARCH_ARM) || defined(ARCH_ARM64)
#include "DebugServer2/Architecture/ARM/HardwareBreakpointManager.h"
#elif defined(ARCH_X86) || defined(ARCH_X86_64)
#include "DebugServer2/Architecture/X86/HardwareBreakpointManager.h"
#else
#error "Architecture not supported."
#endif

namespace ds2 {

#if defined(ARCH_ARM) || defined(ARCH_ARM64)
using Architecture::ARM::HardwareBreakpointManager;
#elif defined(ARCH_X86) || defined(ARCH_X86_64)
using Architecture::X86::HardwareBreakpointManager;
#else
#error "Architecture not supported."
#endif
}

#endif // !__DebugServer2_HardwareBreakpointManager_h
