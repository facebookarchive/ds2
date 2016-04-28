//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/X86/HardwareBreakpointManager.h"
#include "DebugServer2/Target/Process.h"

#define super ds2::BreakpointManager

namespace ds2 {
namespace Architecture {
namespace X86 {

HardwareBreakpointManager::HardwareBreakpointManager(
    Target::ProcessBase *process)
    : super(process) {}

HardwareBreakpointManager::~HardwareBreakpointManager() {}

int HardwareBreakpointManager::maxWatchpoints() {
  // dr0, dr1, dr2, dr3
  return 4;
}
}
}
}
