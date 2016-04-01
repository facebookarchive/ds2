//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/ARM/HardwareBreakpointManager.h"
#include "DebugServer2/Target/Process.h"

#include <algorithm>

#define super ds2::BreakpointManager

namespace ds2 {
namespace Architecture {
namespace ARM {

HardwareBreakpointManager::HardwareBreakpointManager(Target::Process *process)
    : super(process), _maxBreakpoints(_process->getMaxBreakpoints()),
      _maxWatchpoints(_process->getMaxBreakpoints()),
      _maxWatchpointSize(_process->getMaxWatchpointSize()) {}

HardwareBreakpointManager::~HardwareBreakpointManager() {}

int HardwareBreakpointManager::maxWatchpoints() { return _maxWatchpoints; }
}
}
}
