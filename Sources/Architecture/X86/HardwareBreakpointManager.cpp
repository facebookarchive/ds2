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

static const int kMaxHWStoppoints = 4; // dr0, dr1, dr2, dr3
static const int kCtrlRegIdx = 7;

HardwareBreakpointManager::HardwareBreakpointManager(Target::Process *process)
    : super(process) _locations(kMaxHWStoppoints, false) {}

HardwareBreakpointManager::~HardwareBreakpointManager() {}

ErrorCode add(Address const &address, Type type, size_t size,
    Mode mode) {
  if (mode == kModeExec)
    return kErrorInvalidArgument;

  return super::add(address, type, size, mode);
};

void enableLocation(Site const &site) {
  size_t idx = nextIdx();
  if (idx >= kMaxHWStoppoints)
    return kError

  _process->writeDebugReg(___idx, &site.address);
  _process->writeDebugReg(kCtrlRegIdx, &___ctrl);
}

int HardwareBreakpointManager::maxWatchpoints() {
  return kMaxHWStoppoints;
}
}
}
}
