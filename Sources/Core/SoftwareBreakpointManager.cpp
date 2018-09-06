//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Core/SoftwareBreakpointManager.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/HexValues.h"
#include "DebugServer2/Utils/Log.h"

#include <cstdlib>

#define super ds2::BreakpointManager

namespace ds2 {

SoftwareBreakpointManager::SoftwareBreakpointManager(
    Target::ProcessBase *process)
    : super(process), _enabled(false) {}

SoftwareBreakpointManager::~SoftwareBreakpointManager() { clear(); }

void SoftwareBreakpointManager::clear() {
  super::clear();
  _insns.clear();
}

ErrorCode SoftwareBreakpointManager::enableLocation(Site const &site,
                                                    Target::Thread *thread) {
  ByteVector opcode;
  ByteVector old;
  ErrorCode error;

  if (thread != nullptr) {
    DS2LOG(Warning, "thread-specific software breakpoints are unsupported");
  }

  getOpcode(site.size, opcode);
  old.resize(opcode.size());
  error = _process->readMemory(site.address, old.data(), old.size());
  if (error != kSuccess) {
    DS2LOG(Error, "cannot enable breakpoint at %" PRI_PTR ", readMemory failed",
           PRI_PTR_CAST(site.address.value()));
    return error;
  }

  error = _process->writeMemory(site.address, opcode.data(), opcode.size());
  if (error != kSuccess) {
    DS2LOG(Error,
           "cannot enable breakpoint at %" PRI_PTR ", writeMemory failed",
           PRI_PTR_CAST(site.address.value()));
    return error;
  }

  DS2LOG(Debug,
         "set breakpoint instruction 0x%s at %" PRI_PTR " (saved insn 0x%s)",
         ToHex(opcode).c_str(), PRI_PTR_CAST(site.address.value()),
         ToHex(old).c_str());

  _insns[site.address] = old;

  return kSuccess;
}

ErrorCode SoftwareBreakpointManager::disableLocation(Site const &site,
                                                     Target::Thread *thread) {
  ErrorCode error;
  ByteVector old = _insns[site.address];

  if (thread != nullptr) {
    DS2LOG(Warning, "thread-specific software breakpoints are unsupported");
  }

  error = _process->writeMemory(site.address, old.data(), old.size());
  if (error != kSuccess) {
    DS2LOG(Error, "cannot restore instruction at %" PRI_PTR,
           PRI_PTR_CAST(site.address.value()));
    return error;
  }

  DS2LOG(Debug, "reset instruction 0x%s at %" PRI_PTR, ToHex(old).c_str(),
         PRI_PTR_CAST(site.address.value()));

  _insns.erase(site.address);

  return kSuccess;
}

void SoftwareBreakpointManager::enable(Target::Thread *thread) {
  super::enable(thread);

  _enabled = true;
}

void SoftwareBreakpointManager::disable(Target::Thread *thread) {
  super::disable(thread);

  _enabled = false;
}

bool SoftwareBreakpointManager::enabled(Target::Thread *thread) const {
  if (thread != nullptr) {
    DS2LOG(Warning, "thread-specific software breakpoints are unsupported");
  }

  return _enabled;
}

bool SoftwareBreakpointManager::fillStopInfo(Target::Thread *thread,
                                             StopInfo &stopInfo) {
  BreakpointManager::Site site;
  int bpIdx = hit(thread, site);
  if (bpIdx < 0) {
    return false;
  }
  stopInfo.reason = StopInfo::kReasonBreakpoint;
  return true;
}
} // namespace ds2
