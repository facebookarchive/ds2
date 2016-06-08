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
#include "DebugServer2/Utils/Log.h"

#include <algorithm>

#define super ds2::BreakpointManager

namespace ds2 {
namespace Architecture {
namespace X86 {

static const int kMaxHWStoppoints = 4; // dr0, dr1, dr2, dr3

HardwareBreakpointManager::HardwareBreakpointManager(
    Target::ProcessBase *process)
    : super(process), _locations(kMaxHWStoppoints, 0) {}

HardwareBreakpointManager::~HardwareBreakpointManager() {}

int HardwareBreakpointManager::maxWatchpoints() { return kMaxHWStoppoints; }

int HardwareBreakpointManager::getAvailableLocation() {
  DS2ASSERT(_locations.size() == kMaxHWStoppoints);

  auto it = std::find(_locations.begin(), _locations.end(), 0);
  if (it == _locations.end()) {
    return -1;
  }

  return (it - _locations.begin());
}
}
}
}
