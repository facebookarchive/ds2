//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/ARM/SoftwareBreakpointManager.h"
#include "DebugServer2/Target/Process.h"

#include <sys/syscall.h>
#include <sys/mman.h>
#include <cstdlib>

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

BreakpointManager *Process::breakpointManager() const {
  if (_breakpointManager == nullptr) {
    const_cast<Process *>(this)->_breakpointManager =
        new Architecture::ARM::SoftwareBreakpointManager(
            reinterpret_cast<Target::Process *>(const_cast<Process *>(this)));
  }

  return _breakpointManager;
}

WatchpointManager *Process::watchpointManager() const { return nullptr; }

bool Process::isSingleStepSupported() const { return true; }

GDBDescriptor const *Process::getGDBRegistersDescriptor() const {
  return &Architecture::ARM64::GDB;
}

LLDBDescriptor const *Process::getLLDBRegistersDescriptor() const {
  return &Architecture::ARM64::LLDB;
}
}
}
}
