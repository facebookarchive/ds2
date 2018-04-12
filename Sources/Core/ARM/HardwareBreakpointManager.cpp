//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Core/HardwareBreakpointManager.h"
#include "DebugServer2/Target/Process.h"

#include <algorithm>

#define super ds2::BreakpointManager

namespace ds2 {

int HardwareBreakpointManager::hit(Target::Thread *thread, Site &site) {
  return -1;
};

ErrorCode HardwareBreakpointManager::enableLocation(Site const &site, int idx,
                                                    Target::Thread *thread) {
  return kErrorUnsupported;
};

ErrorCode HardwareBreakpointManager::disableLocation(int idx,
                                                     Target::Thread *thread) {
  return kErrorUnsupported;
};

size_t HardwareBreakpointManager::maxWatchpoints() {
  return _process->getMaxWatchpoints();
}

ErrorCode HardwareBreakpointManager::isValid(Address const &address,
                                             size_t size, Mode mode) const {
  DS2LOG(Debug, "Trying to set hardware breakpoint on arm");
  return kErrorUnsupported;
}

size_t HardwareBreakpointManager::chooseBreakpointSize() const {
  DS2BUG(
      "Choosing a hardware breakpoint size on ARM is an unsupported operation");
}
} // namespace ds2
