//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/HardwareBreakpointManager.h"
#include "DebugServer2/SoftwareBreakpointManager.h"

#include <cstdlib>
#include <sys/mman.h>
#include <sys/syscall.h>

using ds2::Architecture::GDBDescriptor;
using ds2::Architecture::LLDBDescriptor;

namespace ds2 {
namespace Target {
namespace Linux {

ErrorCode Process::allocateMemory(size_t size, uint32_t protection,
                                  uint64_t *address) {
  return kErrorUnsupported;
}

ErrorCode Process::deallocateMemory(uint64_t address, size_t size) {
  return kErrorUnsupported;
}

SoftwareBreakpointManager *Process::softwareBreakpointManager() const {
  if (_softwareBreakpointManager == nullptr) {
    const_cast<Process *>(this)->_softwareBreakpointManager =
        new Architecture::ARM::SoftwareBreakpointManager(
            reinterpret_cast<Target::Process *>(const_cast<Process *>(this)));
  }

  return _softwareBreakpointManager;
}

HardwareBreakpointManager *Process::hardwareBreakpointManager() const {
  if (_hardwareBreakpointManager == nullptr) {
    const_cast<Process *>(this)->_hardwareBreakpointManager =
        new Architecture::ARM::HardwareBreakpointManager(
            reinterpret_cast<Target::Process *>(const_cast<Process *>(this)));
  }

  return _hardwareBreakpointManager;
}

int Process::getMaxBreakpoints() const { return 0; }

int Process::getMaxWatchpoints() const { return 0; }

int Process::getMaxWatchpointSize() const { return 0; }

GDBDescriptor const *Process::getGDBRegistersDescriptor() const {
  return &Architecture::ARM64::GDB;
}

LLDBDescriptor const *Process::getLLDBRegistersDescriptor() const {
  return &Architecture::ARM64::LLDB;
}
}
}
}
