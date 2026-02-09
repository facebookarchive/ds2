// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

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
