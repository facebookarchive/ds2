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
#include "DebugServer2/Architecture/ARM/SoftwareBreakpointManager.h"

using ds2::Architecture::GDBDescriptor;
using ds2::Architecture::LLDBDescriptor;

namespace ds2 {
namespace Target {
namespace Windows {

SoftwareBreakpointManager *Process::softwareBreakpointManager() const {
  if (_softwareBreakpointManager == nullptr) {
    const_cast<Process *>(this)->_softwareBreakpointManager =
        new Architecture::ARM::SoftwareBreakpointManager(
            reinterpret_cast<Target::Process *>(const_cast<Process *>(this)));
  }

  return _softwareBreakpointManager;
}

int Process::getMaxBreakpoints() const { return 0; }

int Process::getMaxWatchpoints() const { return 0; }

int Process::getMaxWatchpointSize() const { return 0; }

GDBDescriptor const *Process::getGDBRegistersDescriptor() const {
  return &Architecture::ARM::GDB;
}

LLDBDescriptor const *Process::getLLDBRegistersDescriptor() const {
  return &Architecture::ARM::LLDB;
}
}
}
}
