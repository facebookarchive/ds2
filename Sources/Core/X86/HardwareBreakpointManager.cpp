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
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Bits.h"
#include "DebugServer2/Utils/Log.h"

#include <algorithm>

#define super ds2::BreakpointManager

using ds2::Utils::DisableBit;
using ds2::Utils::DisableBits;
using ds2::Utils::EnableBit;

namespace ds2 {

static const int kStatusRegIdx = 6;
static const int kCtrlRegIdx = 7;
static const int kNumDebugRegisters = 8;

size_t HardwareBreakpointManager::maxWatchpoints() {
  return 4; // dr0, dr1, dr2, dr3
}

ErrorCode HardwareBreakpointManager::enableLocation(Site const &site, int idx,
                                                    Target::Thread *thread) {
  ErrorCode error;
  std::vector<uint64_t> debugRegs(kNumDebugRegisters, 0);

  error = readDebugRegisters(thread, debugRegs);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to read CPU state on hw stoppoint enable");
    return error;
  }

  debugRegs[idx] = site.address;
  debugRegs[kStatusRegIdx] = 0;

  error = enableDebugCtrlReg(debugRegs[kCtrlRegIdx], idx, site.mode, site.size);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to enable debug control register");
    return error;
  }

  error = writeDebugRegisters(thread, debugRegs);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to write CPU state on hw stoppoint enable");
    return error;
  }

  return kSuccess;
}

ErrorCode HardwareBreakpointManager::disableLocation(int idx,
                                                     Target::Thread *thread) {
  ErrorCode error;
  std::vector<uint64_t> debugRegs(kNumDebugRegisters, 0);

  error = readDebugRegisters(thread, debugRegs);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to read CPU state on hw stoppoint disable");
    return error;
  }

  debugRegs[idx] = 0;
  error = disableDebugCtrlReg(debugRegs[kCtrlRegIdx], idx);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to disable debug control register");
    return error;
  }

  error = writeDebugRegisters(thread, debugRegs);
  if (error != kSuccess) {
    DS2LOG(Error, "failed to write CPU state on hw stoppoint enable");
    return error;
  }

  return kSuccess;
}

ErrorCode HardwareBreakpointManager::enableDebugCtrlReg(uint64_t &ctrlReg,
                                                        int idx, Mode mode,
                                                        int size) {
  int enableIdx = idx * 2;
#if !defined(OS_WIN32)
  enableIdx += 1;
#endif

  int infoIdx = 16 + (idx * 4);

  // Set G<idx> flag
  // Except on windows, we use L<idx> as global hardware breakpoints
  // are disabled on Windows.
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
      DisableBit(ctrlReg, infoIdx + 2);
      EnableBit(ctrlReg, infoIdx + 3);
      break;
    default:
      DS2LOG(Error, "invalid hardware breakpoint size: %d", size);
      return kErrorInvalidArgument;
    }
  }

  // Make sure to clear top half of the register
  DisableBits(ctrlReg, 32, 64);

  return kSuccess;
}

ErrorCode HardwareBreakpointManager::disableDebugCtrlReg(uint64_t &ctrlReg,
                                                         int idx) {
  int disableIdx = idx * 2;
#if !defined(OS_WIN32)
  disableIdx += 1;
#endif

  // Unset G<idx> flag
  // Except on windows, we use L<idx> as global hardware breakpoints
  // are disabled on Windows.
  DisableBit(ctrlReg, disableIdx);

  // Make sure to clear top half of the register
  DisableBits(ctrlReg, 32, 64);

  return kSuccess;
}

int HardwareBreakpointManager::hit(Target::Thread *thread, Site &site) {
  if (_sites.size() == 0) {
    return -1;
  }

  if (thread->state() != Target::Thread::kStopped) {
    return -1;
  }

  std::vector<uint64_t> debugRegs(kNumDebugRegisters, 0);
  if (readDebugRegisters(thread, debugRegs) != kSuccess) {
    return -1;
  }

  int regIdx = -1;
  for (size_t i = 0; i < maxWatchpoints(); ++i) {
    if (debugRegs[kStatusRegIdx] & (1 << i)) {
      DS2ASSERT(_locations[i] != 0);
      site = _sites.find(_locations[i])->second;
      regIdx = i;
      break;
    }
  }

  debugRegs[kStatusRegIdx] = 0;
  writeDebugRegisters(thread, debugRegs);
  return regIdx;
}

ErrorCode HardwareBreakpointManager::isValid(Address const &address,
                                             size_t size, Mode mode) const {
  switch (size) {
  case 1:
    break;
  case 8:
    DS2LOG(Warning, "8-byte breakpoints not supported on all architectures");
    DS2_FALLTHROUGH;

  case 2:
  case 4:
    if (mode == kModeExec) {
      return kErrorInvalidArgument;
    }
    break;
  default:
    DS2LOG(Debug, "Received unsupported hardware breakpoint size %zu", size);
    return kErrorInvalidArgument;
  }

  if ((mode & kModeExec) && (mode & (kModeRead | kModeWrite))) {
    DS2LOG(Debug, "Trying to set a hardware breakpoint with mixed exec and "
                  "read/write modes");
    return kErrorInvalidArgument;
  }

  if (mode == kModeRead) {
    return kErrorUnsupported;
  }

  return super::isValid(address, size, mode);
}

size_t HardwareBreakpointManager::chooseBreakpointSize() const {
  DS2BUG(
      "Choosing a hardware breakpoint size on x86 is an unsupported operation");
}

ErrorCode HardwareBreakpointManager::readDebugRegisters(
    Target::Thread *thread, std::vector<uint64_t> &regs) const {
  Architecture::CPUState state;

  CHK(thread->readCPUState(state));

#if defined(ARCH_X86)
  for (int i = 0; i < kNumDebugRegisters; ++i) {
    regs[i] = (i == 4 || i == 5) ? 0 : state.dr.dr[i];
  }
#elif defined(ARCH_X86_64)
  for (int i = 0; i < kNumDebugRegisters; ++i) {
    if (state.is32) {
      regs[i] = (i == 4 || i == 5) ? 0 : state.state32.dr.dr[i];
    } else {
      regs[i] = (i == 4 || i == 5) ? 0 : state.state64.dr.dr[i];
    }
  }
#else
#error "Architecture not supported."
#endif

  return kSuccess;
}

ErrorCode HardwareBreakpointManager::writeDebugRegisters(
    Target::Thread *thread, std::vector<uint64_t> &regs) const {
  Architecture::CPUState state;

  CHK(thread->readCPUState(state));

#if defined(ARCH_X86)
  for (int i = 0; i < kNumDebugRegisters; ++i) {
    state.dr.dr[i] = (i == 4 || i == 5) ? 0 : regs[i];
  }
#elif defined(ARCH_X86_64)
  for (int i = 0; i < kNumDebugRegisters; ++i) {
    if (state.is32) {
      state.state32.dr.dr[i] = (i == 4 || i == 5) ? 0 : regs[i];
    } else {
      state.state64.dr.dr[i] = (i == 4 || i == 5) ? 0 : regs[i];
    }
  }
#else
#error "Architecture not supported."
#endif

  return thread->writeCPUState(state);
}
} // namespace ds2
