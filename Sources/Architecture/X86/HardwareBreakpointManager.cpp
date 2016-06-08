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
#include "DebugServer2/Utils/Bits.h"
#include "DebugServer2/Utils/Log.h"

#include <algorithm>

#define super ds2::BreakpointManager

using ds2::Utils::EnableBit;
using ds2::Utils::DisableBit;

namespace ds2 {
namespace Architecture {
namespace X86 {

static const int kMaxHWStoppoints = 4; // dr0, dr1, dr2, dr3

HardwareBreakpointManager::HardwareBreakpointManager(
    Target::ProcessBase *process)
    : super(process), _locations(kMaxHWStoppoints, 0) {}

HardwareBreakpointManager::~HardwareBreakpointManager() {}

ErrorCode HardwareBreakpointManager::add(Address const &address, Type type,
                                         size_t size, Mode mode) {
  DS2ASSERT(_sites.size() <= kMaxHWStoppoints);

  return super::add(address, type, size, mode);
}

int HardwareBreakpointManager::maxWatchpoints() { return kMaxHWStoppoints; }

ErrorCode HardwareBreakpointManager::enableDebugCtrlReg(uint32_t &ctrlReg,
                                                        int idx, Mode mode,
                                                        int size) {
  int enableIdx = 1 + (idx * 2);
  int infoIdx = 16 + (idx * 4);

  // Set G<idx> flag
  EnableBit(ctrlReg, enableIdx);

  // Set R/W<idx> flags
  switch (static_cast<int>(mode)) {
  case kModeExec:
    DisableBit(ctrlReg, infoIdx);
    DisableBit(ctrlReg, infoIdx + 1);
    break;
  case kModeWrite:
    EnableBit(ctrlReg, infoIdx);
    DisableBit(ctrlReg, infoIdx + 1);
    break;
  case kModeRead:
  case kModeRead | kModeWrite:
    EnableBit(ctrlReg, infoIdx);
    EnableBit(ctrlReg, infoIdx + 1);
    break;
  default:
    return kErrorInvalidArgument;
  }

  // Set LEN<idx> flags, always 0 for exec breakpoints
  if (mode == kModeExec) {
    DisableBit(ctrlReg, infoIdx + 2);
    DisableBit(ctrlReg, infoIdx + 3);
  } else {
    switch (size) {
    case 1:
      DisableBit(ctrlReg, infoIdx + 2);
      DisableBit(ctrlReg, infoIdx + 3);
      break;
    case 2:
      EnableBit(ctrlReg, infoIdx + 2);
      DisableBit(ctrlReg, infoIdx + 3);
      break;
    case 4:
      EnableBit(ctrlReg, infoIdx + 2);
      EnableBit(ctrlReg, infoIdx + 3);
      break;
    case 8:
      DS2LOG(Warning, "8-byte hw breakpoints not supported on all devices");
      DisableBit(ctrlReg, infoIdx + 2);
      EnableBit(ctrlReg, infoIdx + 3);
      break;
    default:
      return kErrorInvalidArgument;
    }
  }

  return kSuccess;
}

ErrorCode HardwareBreakpointManager::disableDebugCtrlReg(uint32_t &ctrlReg,
                                                         int idx) {
  // Unset G<idx> flag
  DisableBit(ctrlReg, 1 + (idx * 2));

  return kSuccess;
}

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
